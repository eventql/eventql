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
#ifndef _STX_JSON_JSONRPCREQUEST_H
#define _STX_JSON_JSONRPCREQUEST_H
#include <stdlib.h>
#include <string>
#include <vector>
#include "eventql/util/inspect.h"
#include "eventql/util/json/json.h"

namespace util {
namespace json {

class JSONRPCRequest {
public:
  JSONRPCRequest(JSONObject&& body);

  const std::string& id() const;
  const std::string& method() const;

  JSONObject::const_iterator paramsBegin() const;
  JSONObject::const_iterator paramsEnd() const;

  template <typename T>
  T getArg(size_t index, const std::string& name) const;

protected:
  JSONObject body_;
  JSONObject::const_iterator params_;
  std::string id_;
  std::string method_;
};

} // namespace json
} // namespace util

#include "jsonrpcrequest_impl.h"
#endif
