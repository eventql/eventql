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

TEST(HttpChannel, completed_invoked_before_contentLength_satisfied) {
  DirectExecutor executor;
  MockTransport transport(&executor, [](HttpRequest* request,
                                        HttpResponse* response) {
    response->setStatus(xzero::HttpStatus::Ok);
    response->addHeader("Content-Type", "text/plain");
    response->setContentLength(10);
    response->output()->write("12345");
    response->completed();
  });

  // we expect the connection to terminate on invalid generated
  // response messages

  transport.run(HttpVersion::VERSION_1_1, "GET", "/", {{"Host", "test"}}, "");
  ASSERT_EQ(true, transport.isAborted());
  ASSERT_EQ(true, transport.isCompleted());
  ASSERT_EQ(5, transport.responseBody().size());
}

TEST(HttpChannel, trailer1) {
  DirectExecutor executor;
  MockTransport transport(&executor, [](HttpRequest* request,
                                        HttpResponse* response) {
    response->setStatus(xzero::HttpStatus::Ok);
    response->addHeader("Content-Type", "text/plain");
    response->registerTrailer("Word-Count");
    response->registerTrailer("Mood");
    response->output()->write("Hello, World!\n");
    response->setTrailer("Word-Count", "one");
    response->setTrailer("Mood", "Happy");
    response->completed();
  });

  transport.run(HttpVersion::VERSION_1_1, "GET", "/", {{"Host", "blah"}}, "");

  ASSERT_EQ((int)HttpStatus::Ok, (int)transport.responseInfo().status());
  ASSERT_EQ(2, transport.responseInfo().trailers().size());
  ASSERT_EQ("one", transport.responseInfo().trailers().get("Word-Count"));
  ASSERT_EQ("Happy", transport.responseInfo().trailers().get("Mood"));
}
