// HTTP semantic tests

#include <xzero/http/mock/MockTransport.h>
#include <xzero/http/HttpRequestInfo.h>
#include <xzero/http/HttpResponseInfo.h>
#include <xzero/http/HttpRequest.h>
#include <xzero/http/HttpResponse.h>
#include <xzero/http/HttpOutput.h>
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
  transport.run(HttpVersion::VERSION_1_1, "GET", "/", {}, "");
  ASSERT_EQ(HttpVersion::VERSION_1_1, transport.responseInfo().version());
}

TEST(HttpChannel, sendError504) {
  DirectExecutor executor;
  MockTransport transport(&executor, &sendError504);
  transport.run(HttpVersion::VERSION_1_1, "GET", "/", {}, "");
  ASSERT_EQ(HttpStatus::GatewayTimeout, transport.responseInfo().status());
  ASSERT_TRUE(transport.responseInfo().contentLength() == transport.responseBody().size());
}

TEST(HttpChannel, invalidRequestPath_escapeDocumentRoot1) {
  DirectExecutor executor;
  MockTransport transport(&executor, &handlerOk);
  transport.run(HttpVersion::VERSION_1_1, "GET", "/../../etc/passwd", {}, "");
  ASSERT_EQ(HttpStatus::BadRequest, transport.responseInfo().status());
}

TEST(HttpChannel, invalidRequestPath_escapeDocumentRoot2) {
  DirectExecutor executor;
  MockTransport transport(&executor, &handlerOk);
  transport.run(HttpVersion::VERSION_1_1, "GET", "/..\%2f..\%2fetc/passwd", {}, "");
  ASSERT_EQ(HttpStatus::BadRequest, transport.responseInfo().status());
}

TEST(HttpChannel, invalidRequestPath_injectNullByte1) {
  DirectExecutor executor;
  MockTransport transport(&executor, &handlerOk);
  transport.run(HttpVersion::VERSION_1_1, "GET", "/foo%00", {}, "");
  ASSERT_EQ(HttpStatus::BadRequest, transport.responseInfo().status());
}

TEST(HttpChannel, invalidRequestPath_injectNullByte2) {
  DirectExecutor executor;
  MockTransport transport(&executor, &handlerOk);
  transport.run(HttpVersion::VERSION_1_1, "GET", "/foo%00/bar", {}, "");
  ASSERT_EQ(HttpStatus::BadRequest, transport.responseInfo().status());
}
