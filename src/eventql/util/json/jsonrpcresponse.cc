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
#include "eventql/util/json/jsonrpcresponse.h"

namespace json {

JSONRPCResponse::JSONRPCResponse(
    JSONOutputStream&& output) :
    output_(std::move(output)) {}

void JSONRPCResponse::error(int code, const std::string& message) {
  output_.beginObject();

  output_.addObjectEntry("jsonrpc");
  output_.addString("2.0");
  output_.addComma();

  output_.addObjectEntry("error");
  output_.beginObject();
  output_.addObjectEntry("code");
  output_.addInteger(code);
  output_.addComma();
  output_.addObjectEntry("message");
  output_.addString(message);
  output_.endObject();
  output_.addComma();

  output_.addObjectEntry("id");
  if (id_.length() > 0) {
    output_.addString(id_);
  } else {
    output_.addNull();
  }

  output_.endObject();
}

void JSONRPCResponse::success(
    std::function<void (JSONOutputStream* output)> func) {
  output_.beginObject();

  output_.addObjectEntry("jsonrpc");
  output_.addString("2.0");
  output_.addComma();

  output_.addObjectEntry("id");
  if (id_.length() > 0) {
    output_.addString(id_);
  } else {
    output_.addNull();
  }
  output_.addComma();

  output_.addObjectEntry("result");
  func(&output_);

  output_.endObject();
}

void JSONRPCResponse::setID(const std::string& id) {
  id_ = id;
}

} // namespace json

