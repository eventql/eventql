/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORDMETRIC_HTTPCLIENT_H
#define _FNORDMETRIC_HTTPCLIENT_H
#include "fnord-base/uri.h"
#include "fnord-http/httpresponsefuture.h"
#include "fnord-http/httpconnectionpool.h"
#include "fnord-base/thread/eventloop.h"

namespace fnord {
namespace http {

class HTTPClient {
public:

  HTTPClient();

  HTTPResponse executeRequest(const HTTPRequest& req);

  HTTPResponse executeRequest(
      const HTTPRequest& req,
      const fnord::InetAddr& addr);

  HTTPResponse executeRequest(
      const HTTPRequest& req,
      Function<HTTPResponseFuture* (Promise<HTTPResponse> promise)> factory);

  HTTPResponse executeRequest(
      const HTTPRequest& req,
      const fnord::InetAddr& addr,
      Function<HTTPResponseFuture* (Promise<HTTPResponse> promise)> factory);

protected:
  thread::EventLoop ev_;
  http::HTTPConnectionPool http_;
  std::mutex mutex_;
};

}
}
#endif
