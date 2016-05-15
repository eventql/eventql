/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#include <stdlib.h>
#include <unistd.h>
#include "eventql/util/exception.h"
#include "eventql/util/exceptionhandler.h"
#include "eventql/util/http/httpserver.h"
#include "eventql/util/http/httphandler.h"
#include "eventql/util/http/httprouter.h"
#include "eventql/util/http/httpservice.h"
#include "eventql/util/logging/logger.h"
#include "eventql/util/logging/logoutputstream.h"
#include "eventql/util/thread/threadpool.h"
#include "eventql/util/system/signalhandler.h"

/**
 * Example 1: A simple HTTP Service
 */
class TestService : public stx::http::HTTPService {

  void handleHTTPRequest(
      stx::http::HTTPRequest* req,
      stx::http::HTTPResponse* res) {
    auto res_body = stx::StringUtil::format(
        "pong: $0",
        req->body().toString());

    res->setStatus(stx::http::kStatusOK);
    res->addBody(res_body);
  }

};

/**
 * Example 2: A streaming HTTP Handler
 */
class StreamingTestHandler : public stx::http::HTTPHandler {
public:
  StreamingTestHandler(
      stx::http::HTTPServerConnection* conn,
      stx::http::HTTPRequest* req) :
      conn_(conn),
      req_(req),
      body_len_(0),
      chunks_written_(0) {
    res_.populateFromRequest(*req);
  }

  void handleHTTPRequest() override {
    conn_->readRequestBody([this] (
        const void* data,
        size_t size,
        bool last_chunk) {
      body_len_ += size;

      if (last_chunk) {
        writeResponseHeaders();
      }
    });
  }

  void writeResponseHeaders() {
    res_.setStatus(stx::http::kStatusOK);
    res_.addHeader("Content-Length", stx::StringUtil::toString(5 * 10));
    res_.addHeader("X-Orig-Bodylen", stx::StringUtil::toString(body_len_));

    conn_->writeResponse(
        res_,
        std::bind(&StreamingTestHandler::writeResponseBodyChunk, this));
  }

  void writeResponseBodyChunk() {
    usleep(100000); // sleep for demo

    std::string buf = "blah\n";
    if (++chunks_written_ == 10) {
      conn_->writeResponseBody(
          buf.c_str(),
          buf.length(),
          std::bind(&stx::http::HTTPServerConnection::finishResponse, conn_));
    } else {
      conn_->writeResponseBody(
          buf.c_str(),
          buf.length(),
          std::bind(&StreamingTestHandler::writeResponseBodyChunk, this));
    }
  }

protected:
  size_t body_len_;
  int chunks_written_;
  stx::http::HTTPServerConnection* conn_;
  stx::http::HTTPRequest* req_;
  stx::http::HTTPResponse res_;
};

class StreamingTestHandlerFactory : public stx::http::HTTPHandlerFactory {
public:
  std::unique_ptr<stx::http::HTTPHandler> getHandler(
      stx::http::HTTPServerConnection* conn,
      stx::http::HTTPRequest* req) override {
    return std::unique_ptr<stx::http::HTTPHandler>(
        new StreamingTestHandler(conn, req));
  }
};

int main() {
  stx::system::SignalHandler::ignoreSIGHUP();
  stx::system::SignalHandler::ignoreSIGPIPE();

  stx::CatchAndAbortExceptionHandler ehandler;
  ehandler.installGlobalHandlers();

  stx::log::LogOutputStream logger(stx::OutputStream::getStderr());
  stx::log::Logger::get()->setMinimumLogLevel(stx::log::kTrace);
  stx::log::Logger::get()->listen(&logger);

  stx::thread::ThreadPool thread_pool;
  stx::http::HTTPRouter router;
  stx::http::HTTPServer http_server(&router, &thread_pool);

  TestService ping_example;
  router.addRouteByPrefixMatch("/ping", &ping_example, &thread_pool);

  StreamingTestHandlerFactory streaming_example;
  router.addRouteByPrefixMatch("/stream", &streaming_example);

  http_server.listen(8080);

  for (;;) { usleep(100000); }

  return 0;
}

