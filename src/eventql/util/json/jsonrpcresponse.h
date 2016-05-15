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
#ifndef _STX_JSON_JSONRPCRESPONSE_H
#define _STX_JSON_JSONRPCRESPONSE_H
#include <stdlib.h>
#include <functional>
#include <string>
#include "eventql/util/io/outputstream.h"
#include "eventql/util/json/jsonoutputstream.h"

namespace util {
namespace json {

class JSONRPCResponse {
public:

  static const int kJSONRPCParserError = -32700;
  static const int kJSONRPCPInvalidRequestError = -32600;
  static const int kJSONRPCPMethodNotFoundError = -32601;
  static const int kJSONRPCPInvalidParamsError = -32602;
  static const int kJSONRPCPInternalError = -32603;

  JSONRPCResponse(JSONOutputStream&& output);

  void error(int code, const std::string& message);
  void success(std::function<void (JSONOutputStream* output)> func);

  template <typename T>
  void successAndReturn(const T& ret_val);

  void setID(const std::string& id);

protected:
  std::string id_;
  JSONOutputStream output_;
};

} // namespace json
} // namespace util

#include "jsonrpcresponse_impl.h"
#endif
