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
#include <eventql/sql/expressions/string.h>
#include <eventql/util/stringutil.h>

namespace csql {
namespace expressions {

static void checkArgs(const char* symbol, int argc, int argc_expected) {
  if (argc != argc_expected) {
    RAISE(
        kRuntimeError,
        "wrong number of arguments for %s. expected: %i, got: %i",
        symbol,
        argc_expected,
        argc);
  }
}

void startsWithExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
  checkArgs("STARTSWITH", argc, 2);

  auto val = StringUtil::beginsWith(argv[0].getString(), argv[1].getString());
  *out = SValue(SValue::BoolType(val));
}

void endsWithExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
  checkArgs("ENDSWITH", argc, 2);

  auto val = StringUtil::endsWith(argv[0].getString(), argv[1].getString());
  *out = SValue(SValue::BoolType(val));
}

void upperCaseExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
  checkArgs("UPPERCASE", argc, 1);
  auto val = argv[0].getString();
  StringUtil::toUpper(&val);
  *out = SValue(val);
}

void lowerCaseExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
  checkArgs("LOWERCASE", argc, 1);
  auto val = argv[0].getString();
  StringUtil::toLower(&val);
  *out = SValue(val);
}

void subStringExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
  if (argc < 2 || argc > 3) {
    RAISEF(
        kRuntimeError,
        "wrong number of arguments for substr. expected: 2 or 3, got: $0",
        argc);
  }

  String str = argv[0].getString();
  int64_t strlen = static_cast<int64_t>(str.size());
  int64_t cur = argv[1].getInteger();

  if (cur == 0 || strlen == 0) {
    *out = SValue::newString("");
    return;
  }

  if (cur < 0) {
    cur += strlen;
    if (cur < 0) {
      *out = SValue::newString("");
      return;
    }
  } else {
    cur = std::min(cur - 1, strlen - 1);
  }

  int64_t len = strlen - cur;
  if (argc == 3) {
    len = std::min(argv[2].getInteger(), len);
  }

  if (len == 0) {
    *out = SValue::newString("");
  } else {
    *out = SValue::newString(str.substr(cur, len));
  }
}

void ltrimExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
  if (argc != 1) {
    RAISEF(
        kRuntimeError,
        "wrong number of arguments for ltrim. expected: 1, got: $0",
        argc);
  }

  String str = argv[0].getString();
  while (str.front() == ' ') {
    str = str.substr(1);
  }

  *out = SValue::newString(str);
}

void rtrimExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
  if (argc != 1) {
    RAISEF(
        kRuntimeError,
        "wrong number of arguments for ltrim. expected: 1, got: $0",
        argc);
  }

  String str = argv[0].getString();
  while (str.back() == ' ') {
    str.pop_back();
  }

  *out = SValue::newString(str);
}

}
}
