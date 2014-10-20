
## Unit Tests

### Base

- [x] Base64
- [x] Buffer
- [ ] DateTime
- [x] IdleTimeout
- [ ] WallClock
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

- [x] Pipelined requests
- [x] HTTP/1.1 closed
- [x] HTTP/1.1 keep-alive
- [ ] automatic chunked response when no response content length was given
- [ ] "Connection" header management (custom values)
- [ ] read timeouts should respond 408
- [ ] write timeouts should abort() the connection
- [ ] keepalive timeouts should close() the connection
- [x] HttpParser: request line "GET /path HTTP/1.2\r\n" => 505
- [x] HttpParser: request line "GET /path\r\n" => 400
- [ ] HttpParser: response line
- [x] HttpParser: headers
- [x] HttpParser: body with content-length given
- [x] HttpParser: body with chunked body
- [x] HttpParser: "Foo : Bar\r\n\r\n" => 400 (header with space before colon)
- [x] HttpGenerator: generate response with content-length given
- [x] HttpGenerator: generate response with chunked body
- [ ] 6.6.6: HTTP version not supported
- [ ] rfc7231, 6.5.7. 408 Request Timeout

### HTTP Semantics & Content (rfc7231)

- [ ] request path: URI decoding
- [x] unhandled exceptions should cause a 500
- [x] request path: null-byte injection protection
- [ ] 5.3.4. Accept-Encoding: compress
- [ ] 5.3.4. Accept-Encoding: gzip
- [ ] 5.3.4. Accept-Encoding: identity
- [ ] 5.3.4. Accept-Encoding: *
- [ ] 5.3.5. Accept-Language, via `vector<string> HttpRequest::acceptLanguage()`
- [x] 5.4: "Host" header found more than once => 400
- [x] 5.4: "Host" header missing for http1.1 => 400
- [x] 5.4: "Host" header missing for != http1.1 => 200
- [ ] 5.4: "Host" header contains invalid data => 400
- [ ] 5.1.1: request header "Expect: 100-continue"
- [x] 6.5.1. 400 Bad Request
- [ ] 6.5.10. 411 Length Required
- [ ] 6.5.11. 413 Payload Too Large
- [ ] 6.5.12. 414 URI Too Long
- [ ] 6.5.14. 417 Expectation Failed
- [ ] 7.1.2.  Location, via `HttpResponse::sendPermanentRedirect()`
- [ ] 7.1.2.  Location, via `HttpResponse::sendTemporaryRedirect()`

### Conditional Requests (rfc7232) & Ranged Requests (rfc7273)

Tests `HttpFileHandler` for conditional requests, ranged requests,
and client side cache.

- [ ] ETag, rfc7232
- [ ] Last-Modified, rfc7232
- [ ] If-Match, rfc7232
- [ ] If-None-Match, rfc7232
- [ ] If-Modified-Since, rfc7232
- [ ] If-Unmodified-Since, rfc7232
- [ ] If-Range-Since, rfc7233

### Caching (rfc7234)

... maybe some `HttpCacheHandler` API

### Authentication (rfc7235)

... maybe some `HttpAuthHandler` API

