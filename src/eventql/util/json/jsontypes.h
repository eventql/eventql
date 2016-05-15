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
#ifndef _STX_JSONTYPES_H
#define _STX_JSONTYPES_H
#include <string>
#include <unordered_map>
#include <vector>
#include "eventql/util/buffer.h"
#include "eventql/util/reflect/reflect.h"
#include "eventql/util/traits.h"

namespace json {

enum kTokenType {
  JSON_OBJECT_BEGIN,
  JSON_OBJECT_END,
  JSON_ARRAY_BEGIN,
  JSON_ARRAY_END,
  JSON_STRING,
  JSON_NUMBER,
  JSON_TRUE,
  JSON_FALSE,
  JSON_NULL
};

struct JSONToken {
  JSONToken(kTokenType _type, const std::string& _data);
  JSONToken(kTokenType _type);
  kTokenType type;
  std::string data;
  uint32_t size;
};

typedef std::vector<JSONToken> JSONObject;

} // namespace json

template <>
struct TypeIsVector<json::JSONObject> {
  static const bool value = false;
};

#endif
