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
#include "eventql/util/exception.h"
#include "eventql/util/json/jsonrpcrequest.h"
#include "eventql/util/json/jsonutil.h"

namespace json {

JSONRPCRequest::JSONRPCRequest(
    JSONObject&& body) :
    body_(std::move(body)) {
  auto v_iter = JSONUtil::objectLookup(body_.begin(), body_.end(), "jsonrpc");
  if (v_iter == body_.end()) {
    RAISE(kRuntimeError, "invalid JSONRPC 2.0 request, missing jsonrpc field");
  }

  auto v = fromJSON<std::string>(v_iter, body_.end());
  if (v != "2.0") {
    RAISEF(kRuntimeError, "invalid JSONRPC 2.0 request version: $0", v);
  }

  auto id_iter = JSONUtil::objectLookup(body_.begin(), body_.end(), "id");
  if (id_iter != body_.end()) {
    id_ = fromJSON<std::string>(id_iter, body_.end());
  }

  auto m_iter = JSONUtil::objectLookup(body_.begin(), body_.end(), "method");
  if (m_iter == body_.end()) {
    RAISE(kRuntimeError, "invalid JSONRPC 2.0 request, missing method field");
  } else {
    method_ = fromJSON<std::string>(m_iter, body_.end());
  }

  params_ = JSONUtil::objectLookup(body_.begin(), body_.end(), "params");
  if (params_ == body_.end()) {
    RAISE(kRuntimeError, "invalid JSONRPC 2.0 request, missing params");
  }
}

JSONObject::const_iterator JSONRPCRequest::paramsBegin() const {
  return params_;
}

JSONObject::const_iterator JSONRPCRequest::paramsEnd() const {
  return body_.end();
}

const std::string& JSONRPCRequest::id() const {
  return id_;
}

const std::string& JSONRPCRequest::method() const {
  return method_;
}

} // namespace json

