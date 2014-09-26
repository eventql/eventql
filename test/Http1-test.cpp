// HTTP/1 transport protocol tests

#include <xzero/http/v1/Http1ConnectionFactory.h>
#include <xzero/http/HttpRequest.h>
#include <xzero/http/HttpResponse.h>
#include <xzero/http/HttpOutput.h>
#include <xzero/executor/DirectExecutor.h>
#include <xzero/logging/LogTarget.h>
#include <xzero/logging/LogAggregator.h>
#include <xzero/net/Server.h>
#include <xzero/net/LocalConnector.h>
#include <xzero/Buffer.h>
#include <gtest/gtest.h>

using namespace xzero;

// FIXME HTTP/1.1 with keep-alive) SEGV's on LocalEndPoint.
// TODO test that userapp cannot add invalid headers
//      (e.g. connection level headers, such as Connection, TE,
//      Transfer-Encoding, Keep-Alive)
// FIXME: HttpResponse.setContentType() seems to be ignored.

class ScopedLogger { // {{{
 public:
  ScopedLogger() {
    xzero::LogAggregator::get().setLogLevel(xzero::LogLevel::Trace);
    xzero::LogAggregator::get().setLogTarget(xzero::LogTarget::console());
  }
  ~ScopedLogger() {
    xzero::LogAggregator::get().setLogLevel(xzero::LogLevel::None);
    xzero::LogAggregator::get().setLogTarget(nullptr);
  }
};
// }}}

#define SCOPED_LOGGER() ScopedLogger _scoped_logger_;
#define MOCK_HTTP1_SERVER(server, localConnector, executor)                    \
  xzero::Server server;                                                        \
  xzero::DirectExecutor executor(false);                                       \
  auto localConnector = server.addConnector<xzero::LocalConnector>(&executor); \
  auto http = localConnector->addConnectionFactory<xzero::http1::Http1ConnectionFactory>( \
      64, 128, 5, TimeSpan::fromSeconds(30));                                  \
  http->setHandler([&](HttpRequest* request, HttpResponse* response) {         \
      response->setStatus(HttpStatus::Ok);                                     \
      response->setContentLength(request->path().size() + 1);                  \
      response->setContentType("text/plain");                                  \
      response->output()->write(Buffer(request->path() + "\n"));               \
      response->completed();                                                   \
  });                                                                          \
  server.start();

TEST(Http1, ConnectionClosed) {
  MOCK_HTTP1_SERVER(server, connector, executor);

  std::shared_ptr<LocalEndPoint> ep;
  executor.execute([&] {
    ep = connector->createClient("GET / HTTP/1.1\r\n"
                                 "Connection: close\r\n"
                                 "\r\n");
  });
  // TODO
}

// sends one single request
// TEST(Http1, ConnectionKeepAlive1) { TODO
// }

// sends single request, gets response, sends another one on the same line.
// TEST(Http1, ConnectionKeepAlive2) { TODO
// }

// sends 3 requests pipelined all at once. receives responses in order
TEST(Http1, ConnectionKeepAlive3_pipelined) {
  MOCK_HTTP1_SERVER(server, connector, executor);
  std::shared_ptr<LocalEndPoint> ep;
  executor.execute([&] {
    ep = connector->createClient("GET /one HTTP/1.1\r\nHost: test\r\n\r\n"
                                 "GET /two HTTP/1.1\r\nHost: test\r\n\r\n"
                                 "GET /three HTTP/1.1\r\nHost: test\r\n\r\n");
  });

  // XXX assume keep-alive timeout 60
  // XXX assume max-request-count 100
  ASSERT_EQ(
    "HTTP/1.1 200 Ok\r\n"
    "Server: xzero/0.11.0-dev\r\n"
    "Connection: Keep-Alive\r\n"
    "Keep-Alive: timeout=60, max=99\r\n"
    "Content-Length: 5\r\n"
    "\r\n"
    "/one\n"
    "HTTP/1.1 200 Ok\r\n"
    "Server: xzero/0.11.0-dev\r\n"
    "Connection: Keep-Alive\r\n"
    "Keep-Alive: timeout=60, max=98\r\n"
    "Content-Length: 5\r\n"
    "\r\n"
    "/two\n"
    "HTTP/1.1 200 Ok\r\n"
    "Server: xzero/0.11.0-dev\r\n"
    "Connection: Keep-Alive\r\n"
    "Keep-Alive: timeout=60, max=97\r\n"
    "Content-Length: 7\r\n"
    "\r\n"
    "/three\n",
    ep->output().str());
}

// ensure proper error code on bad request line
TEST(Http1, protocolErrorShouldRaise400) {
  // SCOPED_LOGGER();
  MOCK_HTTP1_SERVER(server, connector, executor);
  std::shared_ptr<LocalEndPoint> ep;
  executor.execute([&] {
    // FIXME HTTP/1.1 (due to keep-alive) SEGV's on LocalEndPoint.
    ep = connector->createClient("GET /\r\n\r\n");
  });
  xzero::Buffer output = ep->output();
  ASSERT_TRUE(output.contains("400 Bad Request"));
}
