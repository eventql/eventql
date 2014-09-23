
# utests

- http/1 transport (rfc7230)
  - transport
    - parser
    - generator
  - channel
    - "GET /path HTTP/1.2\r\n" should respond with 505 (http version not supported)
    - "GET /path\r\n" should return 400
    - "GET /path\r\nFoo : Bar\r\n\r\n" should respond 400
    - 5.4: "Host" header found more than once => 400
    - 5.4: "Host" header missing => 400
    - 5.4: "Host" header contains invalid data => 400
- http/2 transport (rfc draft 14)
  - framing...
  - hpack...
- http semantic (rfc7231)
  - request header fields
  - method: TRACE (x0d lb proxy)
    - Max-Forwards, ...


















