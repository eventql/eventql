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
#include <stdlib.h>
#include <string>
#include "eventql/util/exception.h"
#include "eventql/util/inspect.h"
#include "eventql/util/json/flatjsonreader.h"

namespace util {
namespace json {

FlatJSONReader::FlatJSONReader(
    JSONInputStream&& json) :
    json_(std::move(json)) {}

void FlatJSONReader::read(
    std::function<bool (const JSONPointer&, const std::string&)> func) {
  kTokenType token;
  std::string token_str;
  JSONPointer path;

  if (!json_.readNextToken(&token, &token_str)) {
    RAISE(kRuntimeError, "invalid JSON. unexpected end of stream");
  }

  switch (token) {
    case JSON_OBJECT_BEGIN:
      readObject(func, &path);
      return;

    case JSON_ARRAY_BEGIN:
      readArray(func, &path);
      return;

    default:
      RAISE(kRuntimeError, "invalid JSON. root node must be OBJECT or ARRAY");
  }
}

void FlatJSONReader::readObject(
    std::function<bool (const JSONPointer&, const std::string&)> func,
    JSONPointer* path) {
  for (;;) {
    kTokenType key_token;
    std::string key_str;

    if (!json_.readNextToken(&key_token, &key_str)) {
      RAISE(kRuntimeError, "invalid JSON. unexpected end of stream");
    }

    switch (key_token) {
      case JSON_OBJECT_END:
        return;

      case JSON_STRING:
      case JSON_NUMBER:
        path->push(key_str);
        break;

      case JSON_TRUE:
        path->push("true");
        break;

      case JSON_FALSE:
        path->push("false");
        break;

      case JSON_NULL:
        path->push("null");
        break;

      default:
        RAISE(
            kRuntimeError,
            "invalid JSON. object key must be of type string, number, boolean "
            "or null");
    }

    kTokenType value_token;
    std::string value_str;

    if (!json_.readNextToken(&value_token, &value_str)) {
      RAISE(kRuntimeError, "invalid JSON. unexpected end of stream");
    }

    switch (value_token) {
      case JSON_OBJECT_BEGIN:
        readObject(func, path);
        break;

      case JSON_ARRAY_BEGIN:
        readArray(func, path);
        break;

      case JSON_STRING:
      case JSON_NUMBER:
      case JSON_TRUE:
      case JSON_FALSE:
      case JSON_NULL:
        func(*path, value_str);
        break;

      default:
        RAISE(
            kRuntimeError,
            "invalid JSON. object key must be of type string, number, boolean "
            "or null");
    }

    path->pop();
  }
}

void FlatJSONReader::readArray(
    std::function<bool (const JSONPointer&, const std::string&)> func,
    JSONPointer* path) {
  for (int i = 0; ; i++) {
    kTokenType elem_token;
    std::string elem_str;

    if (!json_.readNextToken(&elem_token, &elem_str)) {
      RAISE(kRuntimeError, "invalid JSON. unexpected end of stream");
    }

    path->push(std::to_string(i));

    switch (elem_token) {
      case JSON_ARRAY_END:
        return;

      case JSON_OBJECT_BEGIN:
        readObject(func, path);
        break;

      case JSON_ARRAY_BEGIN:
        readArray(func, path);
        break;

      case JSON_STRING:
      case JSON_NUMBER:
        func(*path, elem_str);
        break;

      case JSON_TRUE:
        func(*path, "true");
        break;

      case JSON_FALSE:
        func(*path, "false");
        break;

      case JSON_NULL:
        func(*path, "null");
        break;

      default:
        RAISE(
            kRuntimeError,
            "invalid JSON. unexpected token");
    }

    path->pop();
  }
}

} // namespace json
} // namespace util

