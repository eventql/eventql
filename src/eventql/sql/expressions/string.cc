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
#include <eventql/sql/expressions/string.h>
#include <eventql/util/stringutil.h>

namespace csql {
namespace expressions {

void lcase_call(sql_txn* ctx, VMStack* stack) {
  auto str = popString(stack);
  StringUtil::toLower(&str);
  pushString(stack, str);
}

const SFunction lcase(
    { SType::STRING },
    SType::STRING,
    &lcase_call);

void ucase_call(sql_txn* ctx, VMStack* stack) {
  auto str = popString(stack);
  StringUtil::toUpper(&str);
  pushString(stack, str);
}

const SFunction ucase(
    { SType::STRING },
    SType::STRING,
    &ucase_call);

void startswith_call(sql_txn* ctx, VMStack* stack) {
  auto substr = popString(stack);
  auto str = popString(stack);
  auto startswith = StringUtil::beginsWith(str, substr);
  pushBool(stack, startswith);
}

const SFunction startswith(
    { SType::STRING, SType::STRING },
    SType::BOOL,
    &startswith_call);

void endswith_call(sql_txn* ctx, VMStack* stack) {
  auto substr = popString(stack);
  auto str = popString(stack);
  auto endswith = StringUtil::endsWith(str, substr);
  pushBool(stack, endswith);
}

const SFunction endswith(
    { SType::STRING, SType::STRING },
    SType::BOOL,
    &endswith_call);

void ltrim_call(sql_txn* ctx, VMStack* stack) {
  auto str = popString(stack);
  StringUtil::ltrim(&str);
  pushString(stack, str);
}

const SFunction ltrim(
    { SType::STRING },
    SType::STRING,
    &ltrim_call);

void rtrim_call(sql_txn* ctx, VMStack* stack) {
  auto str = popString(stack);
  StringUtil::rtrim(&str);
  pushString(stack, str);
}

const SFunction rtrim(
    { SType::STRING },
    SType::STRING,
    &rtrim_call);
//
//void subStringExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
//  if (argc < 2 || argc > 3) {
//    RAISEF(
//        kRuntimeError,
//        "wrong number of arguments for substr. expected: 2 or 3, got: $0",
//        argc);
//  }
//
//  String str = argv[0].getString();
//  int64_t strlen = static_cast<int64_t>(str.size());
//  int64_t cur = argv[1].getInteger();
//
//  if (cur == 0 || strlen == 0) {
//    *out = SValue::newString("");
//    return;
//  }
//
//  if (cur < 0) {
//    cur += strlen;
//    if (cur < 0) {
//      *out = SValue::newString("");
//      return;
//    }
//  } else {
//    cur = std::min(cur - 1, strlen - 1);
//  }
//
//  int64_t len = strlen - cur;
//  if (argc == 3) {
//    len = std::min(argv[2].getInteger(), len);
//  }
//
//  if (len == 0) {
//    *out = SValue::newString("");
//  } else {
//    *out = SValue::newString(str.substr(cur, len));
//  }
//}
//
//
//void concatExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
//  std::string str;
//  for (int i = 0; i < argc; ++i) {
//    str += argv[i].getString();
//  }
//
//  *out = SValue::newString(str);
//}

}
}
