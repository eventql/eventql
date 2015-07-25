/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2011-2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORDM_HTTPCONNECTIONPOOL_H
#define _FNORDM_HTTPCONNECTIONPOOL_H
#include <map>
#include <vector>
#include <string>
#include "stx/stdtypes.h"
#include "stx/thread/taskscheduler.h"
#include "stx/net/dnscache.h"
#include "stx/http/httprequest.h"
#include "stx/http/httpresponsefuture.h"
#include "stx/http/httpstats.h"
#include "stx/stats/statsrepository.h"

namespace stx {
namespace http {

class HTTPConnectionPool {
public:
  HTTPConnectionPool(stx::TaskScheduler* scheduler);

  Future<HTTPResponse> executeRequest(const HTTPRequest& req);

  Future<HTTPResponse> executeRequest(
      const HTTPRequest& req,
      const stx::InetAddr& addr);

  Future<HTTPResponse> executeRequest(
      const HTTPRequest& req,
      Function<HTTPResponseFuture* (Promise<HTTPResponse> promise)> factory);

  Future<HTTPResponse> executeRequest(
      const HTTPRequest& req,
      const stx::InetAddr& addr,
      Function<HTTPResponseFuture* (Promise<HTTPResponse> promise)> factory);

  HTTPClientStats* stats();

protected:

  void parkConnection(HTTPClientConnection* conn, InetAddr addr);

  void leaseConnection(
      const stx::InetAddr& addr,
      Promise<HTTPResponse> promise,
      Function<void (HTTPClientConnection* conn)> callback);

  stx::TaskScheduler* scheduler_;

  std::multimap<
      std::string,
      HTTPClientConnection*> connection_cache_;
  std::mutex connection_cache_mutex_;

  stx::net::DNSCache dns_cache_;
  HTTPClientStats stats_;
};

}
}
#endif
