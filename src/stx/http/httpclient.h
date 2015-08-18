/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _libstx_HTTPCLIENT_H
#define _libstx_HTTPCLIENT_H
#include "stx/uri.h"
#include "stx/http/httpresponsefuture.h"
#include "stx/http/httpconnectionpool.h"
#include "stx/thread/eventloop.h"

namespace stx {
namespace http {

class HTTPClient {
public:

  HTTPClient();

  HTTPResponse executeRequest(const HTTPRequest& req);

  HTTPResponse executeRequest(
      const HTTPRequest& req,
      const stx::InetAddr& addr);

  HTTPResponse executeRequest(
      const HTTPRequest& req,
      Function<HTTPResponseFuture* (Promise<HTTPResponse> promise)> factory);

  HTTPResponse executeRequest(
      const HTTPRequest& req,
      const stx::InetAddr& addr,
      Function<HTTPResponseFuture* (Promise<HTTPResponse> promise)> factory);

protected:
  thread::EventLoop ev_;
  std::mutex mutex_;
  stx::net::DNSCache dns_cache_;
};

}
}
#endif
