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
#include "eventql/util/inspect.h"
#include "eventql/util/stringutil.h"
#include "eventql/util/json/json.h"
#include "eventql/util/json/jsonoutputstream.h"
#include "eventql/util/json/jsonrpc.h"
#include "eventql/util/json/jsonrpchttpadapter.h"
#include "eventql/util/json/jsonrpcresponse.h"
#include "eventql/util/json/jsonrpcrequest.h"

namespace util {
namespace json {

std::unique_ptr<http::HTTPService> JSONRPCHTTPAdapter::make(JSONRPC* rpc) {
  return std::unique_ptr<http::HTTPService>(new JSONRPCHTTPAdapter(rpc));
}

JSONRPCHTTPAdapter::JSONRPCHTTPAdapter(
    JSONRPC* json_rpc,
    const std::string path /* = "/rpc" */) :
    json_rpc_(json_rpc),
    path_(path) {
  StringUtil::stripTrailingSlashes(&path_);
}

void JSONRPCHTTPAdapter::handleHTTPRequest(
    http::HTTPRequest* request,
    http::HTTPResponse* response) {
  if (!StringUtil::beginsWith(request->uri(), path_)) {
    response->setStatus(http::kStatusNotFound);
    response->addBody("not found");
    return;
  }

  response->setStatus(http::kStatusOK);
  JSONRPCResponse res(response->getBodyOutputStream());

  if (request->method() != http::HTTPRequest::M_POST) {
    res.error(
        JSONRPCResponse::kJSONRPCPInvalidRequestError,
        "HTTP method must be POST");
    return;
  }

  try {
    JSONRPCRequest req(parseJSON(request->body()));
    res.setID(req.id());
    json_rpc_->dispatch(&req, &res);
  } catch (const util::Exception& e) {
    res.error(
        JSONRPCResponse::kJSONRPCPInternalError,
        e.getMessage());
  }
}


} // namespace json
} // namespace util

