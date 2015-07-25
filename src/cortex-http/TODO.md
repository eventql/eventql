## Incomplete TODO items

### Intermediate 0

- extend FCGI connector to tweak maxKeepAlive (just like in HTTP/1)
  - make sure the keepalive-timeout is fired correctly
    - test cancellation (due to io) and fire (due to timeout).
- use wireshark to get binary expected data for the Parser & Generator tests
- easy enablement of FCGI in all http examples.

### Milestone 1

- [x] FCGI transport
- [x] SSL: basic `SslConnector` & `SslEndPoint`
- [x] SSL: finish SNI support
- [x] SSL: NPN support
- [x] SSL: ALPN support
- [x] SSL: SslConnector to select ConnectionFactory based on NPN/ALPN
- [x] UdpConnector
- [ ] improve timeout management (ideally testable)
- [ ] improve (debug) logging facility

### Milestone 2

- [ ] SSL: ability to setup a certificate password challenge callback
- [ ] Process
- [ ] write full tests for HttpFileHandler using MockTransport
- [ ] LinuxScheduler (using epoll, timerfd, eventfd)
- [ ] net: improved EndPoint timeout handling
      (distinguish between read/write/keepalive timeouts)
- [ ] `HttpTransport::onInterestFailure()` => `(factory || connector)->report(this, error);`
- [ ] `InetEndPoint::wantFill()` to honor `TCP_DEFER_ACCEPT`
- [ ] chunked request trailer support & unit test
- [ ] doxygen: how to document a group of functions all at once (or, how to copydoc)
- [ ] test: call completed() before contentLength is satisfied in non-chunked mode (shall be transport generic)
- [ ] test: attempt to write more data than contentLength in non-chunked mode (shall be transport generic)

### Usage Ideas

- FnordMetric/2
- port x0d to use this library instead
  - write a dedicated haproxy-like load balancer
- http2-to-http1 proxy
x
