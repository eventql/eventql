// This file is part of the "x0" project, http://xzero.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

// HTTP semantic tests

#include <xzero/http/mock/MockTransport.h>
#include <xzero/http/HttpRequestInfo.h>
#include <xzero/http/HttpResponseInfo.h>
#include <xzero/http/HttpRequest.h>
#include <xzero/http/HttpResponse.h>
#include <xzero/http/HttpOutput.h>
#include <xzero/http/BadMessage.h>
#include <xzero/http/HttpFileHandler.h>
#include <xzero/executor/DirectExecutor.h>
#include <xzero/Buffer.h>
#include <gtest/gtest.h>

using namespace xzero;

static std::string generateBoundaryID() {
  return "HelloBoundaryID";
}

void staticfileHandler(HttpRequest* request, HttpResponse* response) {
  const std::string docroot = ".";
  HttpFileHandler staticfile(true, true, true,
                             "/etc/mime.types",
                             "application/unknown",
                             &generateBoundaryID);

  if (staticfile.handle(request, response, docroot))
    return;

  response->setStatus(HttpStatus::NotFound);
  response->completed();
}

TEST(HttpFileHandler, GET_FileNotFound) {
  DirectExecutor executor;
  MockTransport transport(&executor, &staticfileHandler);
  transport.run(HttpVersion::VERSION_1_1, "GET", "/notfound.txt",
      {{"Host", "test"}}, "");
  ASSERT_EQ(HttpVersion::VERSION_1_1, transport.responseInfo().version());
  ASSERT_EQ(404, static_cast<int>(transport.responseInfo().status()));
}

