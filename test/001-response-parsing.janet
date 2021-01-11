(import yanka)

(assert (not (nil? (peg/match yanka/response "HTTP/1.1 200 OK\r\nContent-Length: 100\r\n\r\nUwU"))) "Parsing synthetic response failed")

(assert (not (nil? (yanka/- "http://google.com/"))) "Parsing real response failed")
