/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stdlib.h>
#include <unistd.h>
#include "stx/exception.h"
#include "stx/exceptionhandler.h"
#include "stx/http/httpserver.h"
#include "stx/http/httphandler.h"
#include "stx/http/httprouter.h"
#include "stx/http/httpservice.h"
#include "stx/logging/logger.h"
#include "stx/logging/logoutputstream.h"
#include "stx/thread/threadpool.h"
#include "stx/system/signalhandler.h"

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

