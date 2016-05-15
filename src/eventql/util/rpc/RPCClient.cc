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
#include "eventql/util/logging.h"
#include "eventql/util/rpc/RPCClient.h"

HTTPRPCClient::HTTPRPCClient(
    TaskScheduler* sched) :
    http_pool_(sched, nullptr) {}

void HTTPRPCClient::call(const URI& uri, RefPtr<AnyRPC> rpc) {
  http::HTTPRequest http_req(http::HTTPRequest::M_POST, uri.path());

  http_req.addBody(rpc->encoded());
  http_req.setHeader("Host", uri.hostAndPort());

  auto http_future = http_pool_.executeRequest(http_req);

#ifndef STX_NOTRACE
  logTrace(
      "http.rpcclient",
      "executing RPC via HTTP request\n    id=$2\n    method=$1\n    uri=$0",
      uri.toString(),
      rpc->method(),
      (void*) rpc.get());
#endif

  http_future.onSuccess([uri, rpc] (const http::HTTPResponse& resp) {
    if (resp.statusCode() < 200 || resp.statusCode() >= 300) {
      auto errmsg = StringUtil::format(
          "got non-200 HTTP response code: $0",
          resp.body().toString());

#ifndef STX_NOTRACE
    logTrace(
        "http.rpcclient",
        "RPC via HTTP request id=$0: Failed: $1",
        (void*) rpc.get(),
        errmsg);
#endif

      rpc->error(Status(eRPCError, errmsg));
      return;
    }

#ifndef STX_NOTRACE
    logTrace(
        "http.rpcclient",
        "RPC via HTTP request id=$0: Success, got $1 bytes response",
        (void*) rpc.get(),
        resp.body().size());
#endif
    rpc->ready(resp.body());
  });

  http_future.onFailure([uri, rpc] (const Status& status) mutable{
#ifndef STX_NOTRACE
    logTrace(
        "http.rpcclient",
        "RPC via HTTP request id=$0: Failed: $1",
        (void*) rpc.get(),
        status);
#endif

    rpc->error(status);
  });
}

