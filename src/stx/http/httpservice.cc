/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/inspect.h>
#include <stx/logging.h>
#include "stx/http/httpserverconnection.h"
#include <stx/http/httpservice.h>
#include <stx/http/HTTPResponseStream.h>

namespace stx {
namespace http {

void HTTPService::handleHTTPRequest(
    RefPtr<HTTPRequestStream> req_stream,
    RefPtr<HTTPResponseStream> res_stream) {
  const auto& req = req_stream->request();

  HTTPResponse res;
  res.populateFromRequest(req);

  handleHTTPRequest(const_cast<HTTPRequest*>(&req), &res);

  auto body_size = res.body().size();
  if (body_size > 0) {
    res.setHeader("Content-Length", StringUtil::toString(body_size));
  }

  res_stream->writeResponse(res);
}

HTTPServiceHandler::HTTPServiceHandler(
    StreamingHTTPService* service,
    TaskScheduler* scheduler,
    HTTPServerConnection* conn,
    HTTPRequest* req) :
    service_(service),
    scheduler_(scheduler),
    conn_(conn),
    req_(req) {}

void HTTPServiceHandler::handleHTTPRequest() {
  if (service_->isStreaming()) {
    dispatchRequest();
  } else {
    conn_->readRequestBody([this] (
        const void* data,
        size_t size,
        bool last_chunk) {
      req_->appendBody((char *) data, size);

      if (last_chunk) {
        dispatchRequest();
      }
    });
  }
}

void HTTPServiceHandler::dispatchRequest() {
  auto runnable = [this] () {
    auto res_stream = new HTTPResponseStream(conn_);
    res_stream->incRef();

    RefPtr<HTTPRequestStream> req_stream(new HTTPRequestStream(*req_, conn_));

    try {
      service_->handleHTTPRequest(req_stream.get(), res_stream);
    } catch (const std::exception& e) {
      logError("http.server", e, "Error while processing HTTP request");

      if (res_stream->isOutputStarted()) {
        res_stream->finishResponse();
      } else {
        http::HTTPResponse res;
        res.populateFromRequest(req_stream->request());
        res.setStatus(http::kStatusInternalServerError);
        res.addBody("server error");
        res_stream->writeResponse(res);
      }
    }
  };

  if (scheduler_ == nullptr) {
    runnable();
  } else {
    scheduler_->run(runnable);
  }
}

}
}

