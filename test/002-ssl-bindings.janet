(import "build/ssl")

(def s (net/connect "google.com" 443)) # tested socket

(ssl/init)
(assert (= s (ssl/close (ssl/wrap s))) "Socket is not consistent.")

