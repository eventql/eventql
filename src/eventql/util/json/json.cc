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
#include <stack>
#include <string>
#include "eventql/util/inspect.h"
#include "eventql/util/UnixTime.h"
#include "eventql/util/json/json.h"
#include "eventql/util/json/jsoninputstream.h"

namespace stx {
namespace json {

JSONToken::JSONToken(
    kTokenType _type,
    const std::string& _data) :
    type(_type),
    data(_data),
    size(1) {}

JSONToken::JSONToken(
    kTokenType _type) :
    type(_type),
    size(1) {}

JSONObject parseJSON(const std::string& json_str) {
  JSONInputStream json(StringInputStream::fromString(json_str));
  return parseJSON(&json);
}

JSONObject parseJSON(const stx::Buffer& json_buf) {
  JSONInputStream json(BufferInputStream::fromBuffer(&json_buf));
  return parseJSON(&json);
}

JSONObject parseJSON(JSONInputStream* json) {
  JSONObject obj;
  std::stack<size_t> stack;

  kTokenType token;
  std::string token_str;
  while (json->readNextToken(&token, &token_str)) {
    switch (token) {
      case JSON_ARRAY_BEGIN:
      case JSON_OBJECT_BEGIN:
        obj.emplace_back(token);
        stack.push(obj.size() - 1);
        break;

      case JSON_ARRAY_END:
      case JSON_OBJECT_END: {
        obj.emplace_back(token);

        if (stack.empty()) {
          RAISE(kParseError, "unbalanced braces");
        }

        obj[stack.top()].size = obj.size() - stack.top();
        stack.pop();
        break;
      }

      case JSON_STRING:
      case JSON_NUMBER:
        obj.emplace_back(token, token_str);
        break;

      case JSON_TRUE:
      case JSON_FALSE:
      case JSON_NULL:
        obj.emplace_back(token);
        break;
    }

    token_str.clear();
  }

  if (!stack.empty()) {
    RAISE(kRuntimeError, "invalid JSON. unexpected end of stream");
  }

  return obj;
}

template <>
std::string fromJSONImpl(
    std::vector<JSONToken>::const_iterator begin,
    std::vector<JSONToken>::const_iterator end) {
  if (begin == end) {
    RAISE(kIndexError);
  }

  switch (begin->type) {
    case JSON_STRING:
    case JSON_NUMBER:
      return begin->data;

    case JSON_TRUE:
      return "true";

    case JSON_FALSE:
      return "false";

    case JSON_NULL:
      return "null";

    default:
      RAISEF(kParseError, "can't convert $0 to string", begin->type);

  }
}

template <>
int fromJSONImpl(
    std::vector<JSONToken>::const_iterator begin,
    std::vector<JSONToken>::const_iterator end) {
  if (begin == end) {
    RAISE(kIndexError);
  }

  switch (begin->type) {
    case JSON_STRING:
    case JSON_NUMBER:
      try {
        return std::stoi(begin->data);
      } catch (std::exception& e) {
        /* fallthrough */
      }

    default:
      RAISEF(
          kParseError,
          "can't convert $0 ($1) to integer",
          begin->type,
          begin->data);

  }
}

template <>
unsigned int fromJSONImpl(
    std::vector<JSONToken>::const_iterator begin,
    std::vector<JSONToken>::const_iterator end) {
  if (begin == end) {
    RAISE(kIndexError);
  }

  switch (begin->type) {
    case JSON_STRING:
    case JSON_NUMBER:
      try {
        return std::stoul(begin->data);
      } catch (std::exception& e) {
        /* fallthrough */
      }

    default:
      RAISEF(
          kParseError,
          "can't convert $0 ($1) to integer",
          begin->type,
          begin->data);

  }
}



template <>
unsigned long long fromJSONImpl(
    std::vector<JSONToken>::const_iterator begin,
    std::vector<JSONToken>::const_iterator end) {
  if (begin == end) {
    RAISE(kIndexError);
  }

  switch (begin->type) {
    case JSON_STRING:
    case JSON_NUMBER:
      try {
        return std::stoull(begin->data);
      } catch (std::exception& e) {
        /* fallthrough */
      }

    default:
      RAISEF(
          kParseError,
          "can't convert $0 ($1) to unsigned long long",
          begin->type,
          begin->data);

  }
}

template <>
unsigned long fromJSONImpl(
    std::vector<JSONToken>::const_iterator begin,
    std::vector<JSONToken>::const_iterator end) {
  if (begin == end) {
    RAISE(kIndexError);
  }

  switch (begin->type) {
    case JSON_STRING:
    case JSON_NUMBER:
      try {
        return std::stoul(begin->data);
      } catch (std::exception& e) {
        /* fallthrough */
      }

    default:
      RAISEF(
          kParseError,
          "can't convert $0 ($1) to unsigned long long",
          begin->type,
          begin->data);

  }
}

template <>
bool fromJSONImpl(
    std::vector<JSONToken>::const_iterator begin,
    std::vector<JSONToken>::const_iterator end) {
  if (begin == end) {
    RAISE(kIndexError);
  }

  switch (begin->type) {
    case JSON_TRUE:
      return true;

    case JSON_FALSE:
      return false;

    default:
      RAISEF(
          kParseError,
          "can't convert $0 ($1) to bool",
          begin->type,
          begin->data);

  }
}

template <>
UnixTime fromJSONImpl(
    std::vector<JSONToken>::const_iterator begin,
    std::vector<JSONToken>::const_iterator end) {
  return UnixTime(fromJSONImpl<uint64_t>(begin, end));
}

template <>
JSONObject fromJSONImpl(
    std::vector<JSONToken>::const_iterator begin,
    std::vector<JSONToken>::const_iterator end) {
  if (begin + begin->size > end) {
    RAISE(kIndexError);
  }

  return JSONObject(begin, begin + begin->size);
}

template <>
HashMap<String, String> fromJSONImpl(
    std::vector<JSONToken>::const_iterator begin,
    std::vector<JSONToken>::const_iterator end) {
  HashMap<String, String> map;

  if (begin == end || begin + begin->size > end) {
    RAISE(kIndexError);
  }

  if (begin->type != json::JSON_OBJECT_BEGIN) {
    RAISEF(kParseError, "expected JSON_OBJECT_BEGIN, got: $0", begin->type);
  }

  for (++begin; begin != end;) {
    String key;

    switch (begin->type) {
      case json::JSON_OBJECT_END:
        return map;

      case json::JSON_STRING:
        key = begin->data;
        ++begin;
        break;

      default:
        RAISEF(kParseError, "expected JSON_STRING, got: $0", begin->type);
    }

    switch (begin->type) {
      case json::JSON_STRING:
        map[key] = begin->data;
        ++begin;
        break;

      case json::JSON_NULL:
        map[key] = "";
        ++begin;
        break;

      default:
        RAISEF(kParseError, "expected JSON_STRING, got: $0", begin->type);
    }
  }

  return map;
}


} // namespace json

template <>
std::string StringUtil::toString(json::kTokenType type) {
  switch (type) {
    case json::JSON_OBJECT_BEGIN: return "JSON_OBJECT_BEGIN";
    case json::JSON_OBJECT_END: return "JSON_OBJECT_END";
    case json::JSON_ARRAY_BEGIN: return "JSON_ARRAY_BEGIN";
    case json::JSON_ARRAY_END: return "JSON_ARRAY_END";
    case json::JSON_STRING: return "JSON_STRING";
    case json::JSON_NUMBER: return "JSON_NUMBER";
    case json::JSON_TRUE: return "JSON_TRUE";
    case json::JSON_FALSE: return "JSON_FALSE";
    case json::JSON_NULL: return "JSON_NULL";
  }
}

template <>
std::string inspect(const json::JSONToken& token) {
  return StringUtil::format(
      "$0:$1",
      stx::StringUtil::toString(token.type),
      token.data);
}

} // namespace stx
