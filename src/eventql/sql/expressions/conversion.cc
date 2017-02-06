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
#include <eventql/sql/expressions/conversion.h>

namespace csql {
namespace expressions {

void to_nil_uint64_call(sql_txn* ctx, VMStack* stack) {
  popUInt64(stack);
  pushNil(stack);
}

const SFunction to_nil_uint64(
    { SType::UINT64 },
    SType::NIL,
    &to_nil_uint64_call);

void to_nil_int64_call(sql_txn* ctx, VMStack* stack) {
  popInt64(stack);
  pushNil(stack);
}

const SFunction to_nil_int64(
    { SType::INT64 },
    SType::NIL,
    &to_nil_int64_call);

void to_nil_float64_call(sql_txn* ctx, VMStack* stack) {
  popFloat64(stack);
  pushNil(stack);
}

const SFunction to_nil_float64(
    { SType::FLOAT64 },
    SType::NIL,
    &to_nil_float64_call);

void to_nil_bool_call(sql_txn* ctx, VMStack* stack) {
  popBool(stack);
  pushNil(stack);
}

const SFunction to_nil_bool(
    { SType::BOOL },
    SType::NIL,
    &to_nil_bool_call);

void to_nil_string_call(sql_txn* ctx, VMStack* stack) {
  const char* _1;
  size_t _2;
  popString(stack, &_1, &_2);
  pushNil(stack);
}

const SFunction to_nil_string(
    { SType::BOOL },
    SType::NIL,
    &to_nil_string_call);

void to_nil_timestamp64_call(sql_txn* ctx, VMStack* stack) {
  popTimestamp64(stack);
  pushNil(stack);
}

const SFunction to_nil_timestamp64(
    { SType::TIMESTAMP64 },
    SType::NIL,
    &to_nil_timestamp64_call);

void toStringExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
  if (argc != 1) {
    RAISE(
        kRuntimeError,
        "wrong number of arguments for to_string. expected: 1, got: %i", argc);
  }

  if (argv->getType() == SType::STRING) {
    *out = *argv;
  } else {
    *out = argv->toString();
  }
}

void toIntExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
  if (argc != 1) {
    RAISE(
        kRuntimeError,
        "wrong number of arguments for to_string. expected: 1, got: %i", argc);
  }

  if (argv->getType() == SType::INT64) {
    *out = *argv;
  } else {
    *out = argv->toInteger();
  }
}

void toFloatExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
  if (argc != 1) {
    RAISE(
        kRuntimeError,
        "wrong number of arguments for to_string. expected: 1, got: %i", argc);
  }

  if (argv->getType() == SType::FLOAT64) {
    *out = *argv;
  } else {
    *out = argv->toFloat();
  }
}

void toBoolExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
  if (argc != 1) {
    RAISE(
        kRuntimeError,
        "wrong number of arguments for to_string. expected: 1, got: %i", argc);
  }

  if (argv->getType() == SType::BOOL) {
    *out = *argv;
  } else {
    *out = argv->toBool();
  }
}

}
}
