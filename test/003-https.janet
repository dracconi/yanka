(import yanka)

(yanka/exchange-secure "google.com" 443 "GET / HTTP/1.1\r\nHost: google.com\r\n\r\n")

(yanka/- "https://janet-lang.org:443/")
