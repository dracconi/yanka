(import "yanka")

(def s (net/connect "google.com" 80))

(net/write s "GET / HTTP/1.1\r\nHost: google.com\r\n\r\n")
(net/flush s 0)
(yanka/exhaust-response "" ev/read s 4096 100)

(yanka/- "https://google.com")
(pp (yanka/- "https://janet-lang.org"))
