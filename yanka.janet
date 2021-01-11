(def uri
  '{:main (sequence 
            (capture (sequence "http" (? "s") "://") :proto)
            (capture (some (choice :w ".")) :host)
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

(defn- exchange
  "Exchange data over TCP."
  [host port data]
  (when-with [s (net/connect host port :stream)]
    (net/write s data)
    (ev/read (net/flush s 0) 4000)))

(defn- format-headers 
  "Synthesize headers in the text header."
  [headers]
  (string/join
    (map |(string/join $ ": ")
         (pairs headers))
    "\n"))

(defn- synth-req 
  "Assembles message"
  [typ path headers body]
  (string/join [
    (string/join [(string/ascii-upper typ) path "HTTP/1.1"] " ")
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
                      (cond (> (length url) 3) (in url 2) 80)
                      (synth-req
                        typ
                        (cond (> (length url) 3) (in url 3) (in url 2)) 
                        (or (get-tuple args :headers) @{})
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
