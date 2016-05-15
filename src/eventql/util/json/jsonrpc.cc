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
#include "eventql/util/json/jsonrpc.h"
#include "eventql/util/json/jsonrpcrequest.h"
#include "eventql/util/json/jsonrpcresponse.h"

namespace util {
namespace json {

JSONRPC::JSONRPC() {}

void JSONRPC::dispatch(JSONRPCRequest* req, JSONRPCResponse* res) {
  auto iter = handlers_.find(req->method());
  if (iter == handlers_.end()) {
    res->error(
        JSONRPCResponse::kJSONRPCPMethodNotFoundError,
        "method not found: " + req->method());
  } else {
    iter->second(req, res);
  }
}

void JSONRPC::registerMethod(
    const std::string& method,
    std::function<void (JSONRPCRequest* req, JSONRPCResponse* res)> handler) {
  handlers_[method] = handler;
}

} // namespace json
} // namespace util

