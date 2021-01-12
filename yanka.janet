(import "build/ssl")

(def uri
  '{:main (sequence 
            (capture (sequence "http" (? "s")) :proto) "://"
            (capture (* :w (any (choice :w "." "+" "-"))) :host)
            (? (sequence ":" (capture (some :d))))
            (capture (any (choice :w "/" "-")) :path))})

(def response
  ~{:separators (set "()<>@,;:\\\"/[]?={}\x09 ")
    :ctl (set ,(comptime (string/from-bytes ;(range 0 32))))
    :token (some (* (not (choice :separators :ctl)) 1))
    :field-value (any
                   (choice
                     (sequence "\r\n" (choice " " "\x09"))
                     (* (not :ctl) 1)))
    :header (group (sequence (<- :token) ":" (<- :field-value) "\r\n"))
    :main (sequence "HTTP/" :d (? (sequence "." :d)) " "
            '(3 :d) " " (to "\r\n") "\r\n"
            (any :header) "\r\n"
            '(position))})

(defn- get-tuple
  "Get, but works for tuples CL-alike."
  [tup k]
  (if-let [ix (index-of k tup false)]
    (get tup (+ 1 ix)) nil))

(defn- exchange-cleartext
  "Exchange data over TCP."
  [host port data]
  (when-with [s (net/connect host port :stream)]
    (net/write s data)
    (ev/read (net/flush s 0) 4096)))

(defn exchange-secure
  [host port data]
  (let [s (ssl/wrap (net/connect host port :stream))]
    (net/flush (ssl/socket s) 1)
    (ssl/write s data)
    (net/flush (ssl/socket s) 1)
    (let [b (ssl/read s 4096)]
      (net/close (ssl/close s))
      b)))

(defn- exchange
  [host port tls? data]
  (cond tls? (exchange-secure host port data)
        (exchange-cleartext host port data)))

(defn- format-headers 
  "Synthesize headers in the text header."
  [headers]
  (string/join
    (map |(string/join $ ": ")
         (pairs headers))
    "\n"))

(defn default-headers
  [host]
  @{
   "Host" host
   "Accept" "*/*"
  })

(defn- synth-req 
  "Assembles message"
  [typ host path headers body]
  (string/join [
    (string/join [(string/ascii-upper typ) (cond (> (length path) 0) path "/") "HTTP/1.1"] " ")
    (format-headers headers)
    ""
    body
  ] "\r\n"))

(defn parse-response
  "Parses response"
  [resp]
  (let [m (peg/match response resp)
        body-start (1 (reverse m))]
    @{:body (string/slice resp body-start)
      :status (scan-number (in m 0))
      :headers
        (array/slice m 1 (- (length m) 2))}))

(defn make-req
  "Make arbitrary HTTP request."
  [typ urll args]
  (let [url (peg/match uri urll)]
    (parse-response (exchange
                      (in url 1)
                      (cond (> (length url) 3) (in url 2)
                            (cond
                              (= (in url 0) "https") 443 80))
                      (cond
                        (= (in url 0) "https") true
                        (= (in url 0) "http") false
                        (error "Wrong protocol."))
                      (synth-req
                        typ
                        (in url 1)
                        (cond (> (length url) 3) (in url 3) (in url 2)) 
                        (or (get-tuple args :headers) (default-headers (in url 1)))
                        (or (get-tuple args :body) ""))))))

(defn -
  "Send HTTP get request."
  [url &opt & args]
  (make-req :get url args))

(defn post
  "Send HTTP post request."
  [url &opt & args]
  (make-req :post url args))

(defn put 
  "Send HTTP put request."
  [url &opt & args]
  (make-req :put url args))

(defn patch 
  "Send HTTP patch request."
  [url &opt & args]
  (make-req :patch url args))

(defn delete 
  "Send HTTP delete request."
  [url &opt & args]
  (make-req :delete url args))

(defn options 
  "Send HTTP options request."
  [url &opt & args]
  (make-req :options url args))

(ssl/init)
