// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

// HTTP/1 transport protocol tests

#include <cortex-http/http1/ConnectionFactory.h>
#include <cortex-http/HttpRequest.h>
#include <cortex-http/HttpResponse.h>
#include <cortex-http/HttpOutput.h>
#include <cortex-base/executor/DirectExecutor.h>
#include <cortex-base/logging/LogTarget.h>
#include <cortex-base/logging/LogAggregator.h>
#include <cortex-base/net/Server.h>
#include <cortex-base/net/LocalConnector.h>
#include <cortex-base/Buffer.h>
#include <gtest/gtest.h>

using namespace cortex;
using namespace cortex::http;
using namespace cortex::http::http1;

// TODO use the HttpParser to parse the responses into high level abstracts
//      for better testing.
//
// FIXME HTTP/1.1 with keep-alive) SEGV's on LocalEndPoint.
//
// TODO test that userapp cannot add invalid headers
//      (e.g. connection level headers, such as Connection, TE,
//      Transfer-Encoding, Keep-Alive)

class ScopedLogger { // {{{
 public:
  ScopedLogger() {
    cortex::LogAggregator::get().setLogLevel(cortex::LogLevel::Trace);
    cortex::LogAggregator::get().setLogTarget(cortex::LogTarget::console());
  }
  ~ScopedLogger() {
    cortex::LogAggregator::get().setLogLevel(cortex::LogLevel::None);
    cortex::LogAggregator::get().setLogTarget(nullptr);
  }
};
// }}}

static const size_t maxRequestUriLength = 64;
static const size_t maxRequestBodyLength = 128;
static const size_t maxRequestCount = 5;
static const TimeSpan maxKeepAlive = TimeSpan::fromSeconds(30);

#define SCOPED_LOGGER() ScopedLogger _scoped_logger_;
#define MOCK_HTTP1_SERVER(server, localConnector, executor)                     \
  cortex::Server server;                                                        \
  cortex::DirectExecutor executor(false);                                       \
  cortex::WallClock* clock = nullptr;                                           \
  auto localConnector = server.addConnector<cortex::LocalConnector>(&executor); \
  auto http = localConnector->addConnectionFactory<                             \
                                 cortex::http::http1::ConnectionFactory>(  \
      clock, maxRequestUriLength, maxRequestBodyLength, maxRequestCount,        \
      maxKeepAlive);                                                            \
  http->setHandler([&](HttpRequest* request, HttpResponse* response) {          \
      response->setStatus(HttpStatus::Ok);                                      \
      response->setContentLength(request->path().size() + 1);                   \
      response->setHeader("Content-Type", "text/plain");                        \
      response->output()->write(Buffer(request->path() + "\n"),                 \
          std::bind(&HttpResponse::completed, response));                       \
  });                                                                           \
  server.start();

TEST(http_http1_Connection, ConnectionClosed_1_1) {
  MOCK_HTTP1_SERVER(server, connector, executor);

  cortex::RefPtr<LocalEndPoint> ep;
  executor.execute([&] {
    ep = connector->createClient("GET / HTTP/1.1\r\n"
                                 "Connection: close\r\n"
                                 "\r\n");
  });
  ASSERT_TRUE(ep->output().contains("Connection: close"));
}

TEST(http_http1_Connection, ConnectionClosed_1_0) {
  MOCK_HTTP1_SERVER(server, connector, executor);

  cortex::RefPtr<LocalEndPoint> ep;
  executor.execute([&] {
    ep = connector->createClient("GET / HTTP/1.0\r\n"
                                 "\r\n");
  });
  ASSERT_TRUE(ep->output().contains("Connection: close"));
}

// sends one single request
TEST(http_http1_Connection, DISABLED_ConnectionKeepAlive_1_0) {
  MOCK_HTTP1_SERVER(server, connector, executor);

  cortex::RefPtr<LocalEndPoint> ep;
  executor.execute([&] {
    ep = connector->createClient("GET / HTTP/1.0\r\n"
                                 "Connection: Keep-Alive\r\n"
                                 "\r\n");
  });
  // FIXME: SEGV
  ASSERT_TRUE(ep->output().contains("\r\nKeep-Alive:"));
}

#if 0
// sends one single request
TEST(http_http1_Connection, ConnectionKeepAlive_1_1) {
  MOCK_HTTP1_SERVER(server, connector, executor);

  cortex::RefPtr<LocalEndPoint> ep;
  executor.execute([&] {
    ep = connector->createClient("GET / HTTP/1.1\r\n"
                                 "Host: test\r\n"
                                 "\r\n");
    //printf("%s\n", ep->output().str().c_str());
  });
  ASSERT_TRUE(ep->output().contains("\r\nKeep-Alive:"));
}
#endif

// sends single request, gets response, sends another one on the same line.
// TEST(http_http1_Connection, ConnectionKeepAlive2) { TODO
// }

// sends 3 requests pipelined all at once. receives responses in order
TEST(http_http1_Connection, ConnectionKeepAlive3_pipelined) {
  //SCOPED_LOGGER();
  MOCK_HTTP1_SERVER(server, connector, executor);
  cortex::RefPtr<LocalEndPoint> ep;
  executor.execute([&] {
    ep = connector->createClient("GET /one HTTP/1.1\r\nHost: test\r\n\r\n"
                                 "GET /two HTTP/1.1\r\nHost: test\r\n\r\n"
                                 "GET /three HTTP/1.1\r\nHost: test\r\n\r\n");
  });

  // XXX assume keep-alive timeout 30
  // XXX assume max-request-count 5
  ASSERT_EQ(
    "HTTP/1.1 200 Ok\r\n"
    "Content-Type: text/plain\r\n"
    "Server: cortex/0.11.0-dev\r\n"
    "Connection: Keep-Alive\r\n"
    "Keep-Alive: timeout=30, max=4\r\n"
    "Content-Length: 5\r\n"
    "\r\n"
    "/one\n"
    "HTTP/1.1 200 Ok\r\n"
    "Content-Type: text/plain\r\n"
    "Server: cortex/0.11.0-dev\r\n"
    "Connection: Keep-Alive\r\n"
    "Keep-Alive: timeout=30, max=3\r\n"
    "Content-Length: 5\r\n"
    "\r\n"
    "/two\n"
    "HTTP/1.1 200 Ok\r\n"
    "Content-Type: text/plain\r\n"
    "Server: cortex/0.11.0-dev\r\n"
    "Connection: Keep-Alive\r\n"
    "Keep-Alive: timeout=30, max=2\r\n"
    "Content-Length: 7\r\n"
    "\r\n"
    "/three\n",
    ep->output().str());
}

// ensure proper error code on bad request line
TEST(http_http1_Connection, protocolErrorShouldRaise400) {
  // SCOPED_LOGGER();
  MOCK_HTTP1_SERVER(server, connector, executor);
  cortex::RefPtr<LocalEndPoint> ep;
  executor.execute([&] {
    // FIXME HTTP/1.1 (due to keep-alive) SEGV's on LocalEndPoint.
    ep = connector->createClient("GET\r\n\r\n");
  });
  cortex::Buffer output = ep->output();
  ASSERT_TRUE(output.contains("400 Bad Request"));
}
