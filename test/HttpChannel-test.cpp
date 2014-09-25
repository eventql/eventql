// HTTP semantic tests

#include <xzero/http/mock/MockTransport.h>
#include <xzero/http/HttpRequestInfo.h>
#include <xzero/http/HttpResponseInfo.h>
#include <xzero/http/HttpRequest.h>
#include <xzero/http/HttpResponse.h>
#include <xzero/http/HttpOutput.h>
#include <xzero/http/BadMessage.h>
#include <xzero/executor/DirectExecutor.h>
#include <xzero/Buffer.h>
#include <gtest/gtest.h>

using namespace xzero;

void handlerOk(HttpRequest* request, HttpResponse* response) {
  response->setStatus(HttpStatus::Ok);
  response->completed();
}

void sendError504(HttpRequest* request, HttpResponse* response) {
  response->sendError(HttpStatus::GatewayTimeout);
}

TEST(HttpChannel, sameVersion) {
  DirectExecutor executor;
  MockTransport transport(&executor, &handlerOk);
  transport.run(HttpVersion::VERSION_1_0, "GET", "/", {}, "");
  ASSERT_EQ(HttpVersion::VERSION_1_0, transport.responseInfo().version());
}

TEST(HttpChannel, sendError504) {
  DirectExecutor executor;
  MockTransport transport(&executor, &sendError504);
  transport.run(HttpVersion::VERSION_1_0, "GET", "/", {}, "");
  ASSERT_EQ((int)HttpStatus::GatewayTimeout, (int)transport.responseInfo().status());
  ASSERT_TRUE(transport.responseInfo().contentLength() == transport.responseBody().size());
}

TEST(HttpChannel, invalidRequestPath_escapeDocumentRoot1) {
  DirectExecutor executor;
  MockTransport transport(&executor, &handlerOk);
  transport.run(HttpVersion::VERSION_1_0, "GET", "/../../etc/passwd", {}, "");
  ASSERT_EQ(HttpStatus::BadRequest, transport.responseInfo().status());
}

TEST(HttpChannel, invalidRequestPath_escapeDocumentRoot2) {
  DirectExecutor executor;
  MockTransport transport(&executor, &handlerOk);
  transport.run(HttpVersion::VERSION_1_0, "GET", "/..\%2f..\%2fetc/passwd", {}, "");
  ASSERT_EQ(HttpStatus::BadRequest, transport.responseInfo().status());
}

TEST(HttpChannel, invalidRequestPath_injectNullByte1) {
  DirectExecutor executor;
  MockTransport transport(&executor, &handlerOk);
  transport.run(HttpVersion::VERSION_1_0, "GET", "/foo%00", {}, "");
  ASSERT_EQ(HttpStatus::BadRequest, transport.responseInfo().status());
}

TEST(HttpChannel, invalidRequestPath_injectNullByte2) {
  DirectExecutor executor;
  MockTransport transport(&executor, &handlerOk);
  transport.run(HttpVersion::VERSION_1_0, "GET", "/foo%00/bar", {}, "");
  ASSERT_EQ(HttpStatus::BadRequest, transport.responseInfo().status());
}

TEST(HttpChannel, missingHostHeader) {
  DirectExecutor executor;
  MockTransport transport(&executor, &handlerOk);
  transport.run(HttpVersion::VERSION_1_1, "GET", "/", {}, "");
  ASSERT_EQ(HttpStatus::BadRequest, transport.responseInfo().status());

  transport.run(HttpVersion::VERSION_1_0, "GET", "/", {}, "");
  ASSERT_EQ(HttpStatus::Ok, transport.responseInfo().status());

  transport.run(HttpVersion::VERSION_0_9, "GET", "/", {}, "");
  ASSERT_EQ(HttpStatus::Ok, transport.responseInfo().status());
}

TEST(HttpChannel, multipleHostHeaders) {
  DirectExecutor executor;
  MockTransport transport(&executor, &handlerOk);
  transport.run(HttpVersion::VERSION_1_1, "GET", "/", {
      {"Host", "foo"}, {"Host", "bar"}}, "");
  ASSERT_EQ(HttpStatus::BadRequest, transport.responseInfo().status());
}

TEST(HttpChannel, unhandledException1) {
  DirectExecutor executor;
  MockTransport transport(&executor, [](HttpRequest*, HttpResponse*) {
    throw std::runtime_error("me, the unhandled");
  });
  transport.run(HttpVersion::VERSION_1_0, "GET", "/", {}, "");
  ASSERT_EQ(HttpStatus::InternalServerError, transport.responseInfo().status());
  ASSERT_EQ("me, the unhandled", transport.responseInfo().reason());
}

TEST(HttpChannel, unhandledException2) {
  DirectExecutor executor;
  MockTransport transport(&executor, [](HttpRequest*, HttpResponse*) {
    throw 42;
  });
  transport.run(HttpVersion::VERSION_1_0, "GET", "/", {}, "");
  ASSERT_EQ(HttpStatus::InternalServerError, transport.responseInfo().status());
}
