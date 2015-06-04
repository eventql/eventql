/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "fnord-base/exception.h"
#include "fnord-http/httpclient.h"
#include "fnord-http/httpclientconnection.h"
#include "fnord-base/net/tcpconnection.h"

namespace fnord {
namespace http {

HTTPClient::HTTPClient() : http_(&ev_) {}

HTTPResponse HTTPClient::executeRequest(
    const HTTPRequest& req) {
  return executeRequest(
      req,
      [] (Promise<HTTPResponse> promise) {
        return new HTTPResponseFuture(promise);
      });
}

HTTPResponse HTTPClient::executeRequest(
    const HTTPRequest& req,
    const fnord::InetAddr& addr) {
  return executeRequest(
      req,
      addr,
      [] (Promise<HTTPResponse> promise) {
        return new HTTPResponseFuture(promise);
      });
}

HTTPResponse HTTPClient::executeRequest(
    const HTTPRequest& req,
    Function<HTTPResponseFuture* (Promise<HTTPResponse> promise)> factory) {
  std::unique_lock<std::mutex> lk(mutex_);

  auto future = http_.executeRequest(req, factory);

  future.onReady([this] {
    ev_.shutdown();
  });

  ev_.run();
  return future.get();
}

HTTPResponse HTTPClient::executeRequest(
    const HTTPRequest& req,
    const fnord::InetAddr& addr,
    Function<HTTPResponseFuture* (Promise<HTTPResponse> promise)> factory) {
  std::unique_lock<std::mutex> lk(mutex_);

  auto future = http_.executeRequest(req, addr, factory);

  future.onReady([this] {
    ev_.shutdown();
  });

  ev_.run();
  return future.get();
}


}
}
