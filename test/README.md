
## Unit Tests

### Base

- [ ] Base64
- [ ] Buffer
- [ ] DateTime
- [ ] IdleTimeout
- [ ] SystemWallClock
- [ ] TimeSpan
- [ ] Tokenizer
- [ ] executor/DirectLoopExecutor
- [ ] executor/DirectExecutor
- [ ] executor/ThreadedExecutor
- [x] executor/ThreadPool
- [ ] net/Cidr
- [ ] net/EndPointWriter
- [ ] net/InetConnector
- [ ] net/InetEndPoint
- [ ] net/IPAddress
- [ ] net/LocalConnector
- [ ] net/Server
- [ ] support/LibevScheduler
- [ ] support/LibevSelector

### HTTP/1 (rfc7230)

- [ ] Pipelined requests
- [ ] HTTP/1.1 closed
- [ ] HTTP/1.1 keep-alive
- [ ] "Connection" header management (custom values)
- [ ] "GET /path HTTP/1.2\r\n" should respond with 505 (http version not supported)
- [ ] "GET /path\r\n" should return 400
- [ ] "GET /path\r\nFoo : Bar\r\n\r\n" should respond 400 (header with space before colon)
- [ ] read timeouts should respond 408
- [ ] write timeouts should abort() the connection
- [ ] keepalive timeouts should close() the connection

### HTTP Semantics & Content (rfc7231)

- [ ] unhandled exceptions should cause a 500
- [x] request path: null-byte injection protection
- [ ] request path: URI decoding
- [ ] 5.1.1: request header "Expect: 100-continue"
- [ ] 6.6.6: HTTP version not supported
- [ ] 5.4: "Host" header found more than once => 400
- [ ] 5.4: "Host" header missing => 400
- [ ] 5.4: "Host" header contains invalid data => 400

### Conditional Requests (rfc7232) & Ranged Requests (rfc7273)

Tests `HttpFileHandler` for conditional requests, ranged requests,
and client side cache.

### Caching (rfc7234)

... maybe some `HttpCacheHandler` API

### Authentication (rfc7235)

... maybe some `HttpAuthHandler` API

