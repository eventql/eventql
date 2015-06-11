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

HTTPClient::HTTPClient() {}

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
  if (!req.hasHeader("Host")) {
    RAISE(kRuntimeError, "missing Host header");
  }

  auto addr = dns_cache_.resolve(req.getHeader("Host"));
  if (!addr.hasPort()) {
    addr.setPort(80);
  }

  return executeRequest(req, addr, factory);
}

HTTPResponse HTTPClient::executeRequest(
    const HTTPRequest& req,
    const fnord::InetAddr& addr,
    Function<HTTPResponseFuture* (Promise<HTTPResponse> promise)> factory) {
  std::unique_lock<std::mutex> lk(mutex_);

  Promise<HTTPResponse> promise;
  auto http_future = factory(promise);

  try {
    ScopedPtr<HTTPClientConnection> conn(
        new HTTPClientConnection(
            net::TCPConnection::connect(addr),
            &ev_,
            nullptr));

    conn->executeRequest(req, http_future);
    http_future->storeConnection(std::move(conn));
  } catch (const std::exception& e) {
    http_future->onError(e);
  }

  auto future = promise.future();

  ev_.runOnce();
  return future.waitAndGet();
}


}
}
