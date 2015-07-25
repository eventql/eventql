// This file is part of the "x0" project, http://cortex.io/
//   (c) 2009-2014 Christian Parpart <trapni@gmail.com>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

// HTTP semantic tests

#include <cortex-http/mock/Transport.h>
#include <cortex-http/HttpRequestInfo.h>
#include <cortex-http/HttpResponseInfo.h>
#include <cortex-http/HttpRequest.h>
#include <cortex-http/HttpResponse.h>
#include <cortex-http/HttpOutput.h>
#include <cortex-http/BadMessage.h>
#include <cortex-http/HttpFileHandler.h>
#include <cortex-base/io/LocalFileRepository.h>
#include <cortex-base/executor/DirectExecutor.h>
#include <cortex-base/io/FileUtil.h>
#include <cortex-base/MimeTypes.h>
#include <cortex-base/Buffer.h>
#include <gtest/gtest.h>

using namespace cortex;
using namespace cortex::http;

static std::string generateBoundaryID() {
  return "HelloBoundaryID";
}

void staticfileHandler(HttpRequest* request, HttpResponse* response) {
  MimeTypes mimetypes;
  LocalFileRepository repo(mimetypes, "/", true, true, true);
  HttpFileHandler staticfile(&generateBoundaryID);


  auto docroot = FileUtil::realpath(".");
  auto file = repo.getFile(request->path(), docroot);

  if (staticfile.handle(request, response, file))
    return;

  response->setStatus(HttpStatus::NotFound);
  response->completed();
}

TEST(http_HttpFileHandler, GET_FileNotFound) {
  DirectExecutor executor;
  mock::Transport transport(&executor, &staticfileHandler);
  transport.run(HttpVersion::VERSION_1_1, "GET", "/notfound.txt",
      {{"Host", "test"}}, "");
  ASSERT_EQ(HttpVersion::VERSION_1_1, transport.responseInfo().version());
  ASSERT_EQ(404, static_cast<int>(transport.responseInfo().status()));
}

