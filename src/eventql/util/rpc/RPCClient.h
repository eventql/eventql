/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _STX_RPCCLIENT_H
#define _STX_RPCCLIENT_H
#include <functional>
#include <stdlib.h>
#include <string>
#include <unordered_map>
#include <vector>
#include "eventql/util/autoref.h"
#include "eventql/util/thread/taskscheduler.h"
#include "eventql/util/uri.h"
#include "eventql/util/rpc/RPC.h"
#include "eventql/util/http/httpconnectionpool.h"

namespace stx {

class RPCClient {
public:
  virtual ~RPCClient() {}

  virtual void call(const URI& uri, RefPtr<AnyRPC> rpc) = 0;

};

class HTTPRPCClient : public RPCClient {
public:
  HTTPRPCClient(TaskScheduler* sched);
  void call(const URI& uri, RefPtr<AnyRPC> rpc) override;
protected:
  stx::http::HTTPConnectionPool http_pool_;
};

} // namespace stx
#endif
