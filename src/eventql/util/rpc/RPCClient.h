/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
  http::HTTPConnectionPool http_pool_;
};

#endif
