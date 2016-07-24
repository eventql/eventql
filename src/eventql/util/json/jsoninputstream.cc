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
#include "eventql/util/inspect.h"
#include "eventql/util/stringutil.h"
#include "eventql/util/json/jsoninputstream.h"

namespace json {

JSONInputStream::JSONInputStream(
    std::unique_ptr<InputStream> input) :
    input_(std::move(input)),
    cur_(' ') {}

JSONInputStream::JSONInputStream(
    JSONInputStream&& other) :
    input_(std::move(other.input_)),
    cur_(other.cur_) {
  other.cur_ = 0;
}

bool JSONInputStream::readNextToken(
    kTokenType* token_type,
    std::string* token_data) {
  token_data->clear();

  for (;;) {
    switch (cur_) {
      case '{':
        *token_type = JSON_OBJECT_BEGIN;
        advanceCursor();
        return true;

      case '}':
        *token_type = JSON_OBJECT_END;
        advanceCursor();
        return true;

      case '[':
        *token_type = JSON_ARRAY_BEGIN;
        advanceCursor();
        return true;

      case ']':
        *token_type = JSON_ARRAY_END;
        advanceCursor();
        return true;

      case ':':
      case ',':
      case ' ':
      case '\n':
      case '\r':
        advanceCursor();
        break;

      case '-':
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        *token_type = JSON_NUMBER;
        readNumber(token_data);
        return true;

      case '"':
        *token_type = JSON_STRING;
        readString(token_data);
        return true;

      case 't':
        *token_type = JSON_TRUE;
        expectString("true");
        return true;

      case 'f':
        *token_type = JSON_FALSE;
        expectString("false");
        return true;

      case 'n':
        *token_type = JSON_NULL;
        expectString("null");
        return true;

      case 0:
        return false;

      default:
        RAISE(kRuntimeError, "invalid json, unexpected char: %i", cur_);
        return false;

    }
  }
}

void JSONInputStream::readNumber(std::string* dst) {
  for (;;) {
    switch (cur_) {
      case '-':
      case '+':
      case '.':
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
      case 'e':
      case 'E':
        *dst += cur_;
        /* fallthrough */

      case ' ':
        advanceCursor();
        break;

      default:
        return;
    }
  }
}

void JSONInputStream::readString(std::string* dst) {
  for (;;) {
    advanceCursor();

    switch (cur_) {

      case '\\':
        advanceCursor();

        switch (cur_) {

          case '"':
            *dst += "\"";
            break;

          case '\\':
            *dst += "\\";
            break;

          case '/':
            *dst += "/";
            break;

          case 'b':
            *dst += "\b";
            break;

          case 'f':
            *dst += "\f";
            break;

          case 'n':
            *dst += "\n";
            break;

          case 'r':
            *dst += "\r";
            break;

          case 't':
            *dst += "\t";
            break;

          case 'u':
            // FIXME
            advanceCursor();
            advanceCursor();
            advanceCursor();
            advanceCursor();
            break;

          default:
            RAISE(kRuntimeError, "invalid escape sequence");

        }
        break;

      default:
        *dst += cur_;
        break;

      case '"':
        advanceCursor();
        return;

      case 0:
        RAISE(kRuntimeError, "invalid json. unterminated string");
        return;

    }
  }
}

void JSONInputStream::expectString(const std::string& expect) {
  std::string str;
  str += cur_;
  input_->readNextBytes(&str, expect.length() - 1);

  if (str != expect) {
    RAISEF(kRuntimeError, "invalid json. expected '$0', got '$1'", expect, str);
  }

  advanceCursor();
}

void JSONInputStream::advanceCursor() {
  if (!input_->readNextByte(&cur_)) {
    cur_ = 0;
  }
}

} // namespace json

template <>
std::string inspect(const json::kTokenType& token) {
  switch (token) {
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

