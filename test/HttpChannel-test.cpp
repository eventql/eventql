#include <xzero/http/v1/HttpConnectionFactory.h>
#include <xzero/http/HttpRequest.h>
#include <xzero/http/HttpResponse.h>
#include <xzero/executor/DirectExecutor.h>
#include <xzero/logging/LogTarget.h>
#include <xzero/logging/LogAggregator.h>
#include <xzero/net/Server.h>
#include <xzero/net/LocalConnector.h>
#include <xzero/Buffer.h>
#include <gtest/gtest.h>

using namespace xzero;

// FIXME HTTP/1.1 with keep-alive) SEGV's on LocalEndPoint.
// TODO how to deal with bad connection header values? (e.g. "closed" or "foo")
// TODO test that userapp cannot add invalid headers
//      (e.g. connection level headers, such as Connection, TE,
//      Transfer-Encoding, Keep-Alive)

#define MOCK_HTTP1_SERVER(server, localConnector, executor)                    \
  xzero::LogAggregator::get().setLogLevel(xzero::LogLevel::Trace);             \
  xzero::LogAggregator::get().setLogTarget(xzero::LogTarget::console());       \
  xzero::Server server;                                                        \
  xzero::DirectExecutor executor(false);                                       \
  auto localConnector = server.addConnector<xzero::LocalConnector>(&executor); \
  auto http = localConnector->addConnectionFactory<xzero::http1::HttpConnectionFactory>(); \
  http->setHandler([&](HttpRequest* request, HttpResponse* response) {         \
      response->setStatus(HttpStatus::Ok);                                     \
      response->setContentLength(0);                                           \
      response->completed();                                                   \
  });                                                                          \
  server.start();

TEST(HttpChannel, http1_ConnectionClosed) {
  xzero::LogAggregator::get().setLogLevel(xzero::LogLevel::Trace);
  xzero::LogAggregator::get().setLogTarget(xzero::LogTarget::console());

  MOCK_HTTP1_SERVER(server, connector, executor);

  std::shared_ptr<LocalEndPoint> ep;
  executor.execute([&] {
    ep = connector->createClient("GET / HTTP/1.1\r\n"
                                 "Connection: close\r\n"
                                 "\r\n");
  });
  printf("result: %s\n", ep->output().c_str());
}

// sends one single request
TEST(HttpChannel, http1_ConnectionKeepAlive1) {
}

// sends single request, gets response, sends another one on the same line.
TEST(HttpChannel, http1_ConnectionKeepAlive2) {
}

// sends 3 requests pipelined all at once. receives responses in order
TEST(HttpChannel, http1_ConnectionKeepAlive3_pipelined) {
}

// ensure proper error code on bad request line
TEST(HttpChannel, protocolErrorShouldRaise400) {
  MOCK_HTTP1_SERVER(server, connector, executor);

  std::shared_ptr<LocalEndPoint> ep;
  executor.execute([&] {
    // FIXME HTTP/1.1 (due to keep-alive) SEGV's on LocalEndPoint.
    ep = connector->createClient("GET / HTTP/1.0\r\n\r\n");
  });
  printf("result: %s\n", ep->output().c_str());
}
