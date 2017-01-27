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
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <eventql/sql/expressions/math.h>
#include <eventql/sql/transaction.h>

namespace csql {
namespace expressions {

void addExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
  if (argc != 2) {
    RAISE(
        kRuntimeError,
        "wrong number of arguments for add. expected: 2, got: %i", argc);
  }

  SValue* lhs = argv;
  SValue* rhs = argv + 1;

  if (lhs->getType() == SType::NIL || rhs->getType() == SType::NIL) {
    *out = SValue();
    return;
  }

  if (!lhs->isConvertibleToNumeric() || !rhs->isConvertibleToNumeric()) {
    *out = SValue(lhs->getString() + rhs->getString());
    return;
  }

  SValue lhs_num = lhs->toNumeric();
  SValue rhs_num = rhs->toNumeric();

  if (lhs_num.getType() == SType::INT64 && rhs_num.getType() == SType::INT64) {
    *out = SValue((int64_t) (lhs_num.getInteger() + rhs_num.getInteger()));
  } else {
    *out = SValue((double) (lhs_num.getFloat() + rhs_num.getFloat()));
  }
}

void subExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
  if (argc != 2) {
    RAISE(
        kRuntimeError,
        "wrong number of arguments for sub. expected: 2, got: %i", argc);
  }

  SValue* lhs = argv;
  SValue* rhs = argv + 1;

  if (lhs->getType() == SType::NIL || rhs->getType() == SType::NIL) {
    *out = SValue();
    return;
  }

  if (!lhs->isConvertibleToNumeric() || !rhs->isConvertibleToNumeric()) {
    RAISE(kRuntimeError, "can't subtract %s and %s",
      lhs->getTypeName(),
      rhs->getTypeName());
  }

  SValue lhs_num = lhs->toNumeric();
  SValue rhs_num = rhs->toNumeric();

  if (lhs_num.getType() == SType::INT64 && rhs_num.getType() == SType::INT64) {
    *out = SValue((int64_t) (lhs_num.getInteger() - rhs_num.getInteger()));
  } else {
    *out = SValue((double) (lhs_num.getFloat() - rhs_num.getFloat()));
  }
}

void mulExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
  if (argc != 2) {
    RAISE(
        kRuntimeError,
        "wrong number of arguments for mul. expected: 2, got: %i", argc);
  }

  SValue* lhs = argv;
  SValue* rhs = argv + 1;

  if (lhs->getType() == SType::NIL || rhs->getType() == SType::NIL) {
    *out = SValue();
    return;
  }

  if (!lhs->isConvertibleToNumeric() || !rhs->isConvertibleToNumeric()) {
    RAISE(kRuntimeError, "can't multiply %s and %s",
      lhs->getTypeName(),
      rhs->getTypeName());
  }

  SValue lhs_num = lhs->toNumeric();
  SValue rhs_num = rhs->toNumeric();

  if (lhs_num.getType() == SType::INT64 && rhs_num.getType() == SType::INT64) {
    *out = SValue((int64_t) (lhs_num.getInteger() * rhs_num.getInteger()));
  } else {
    *out = SValue((double) (lhs_num.getFloat() * rhs_num.getFloat()));
  }
}

void divExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
  if (argc != 2) {
    RAISE(
        kRuntimeError,
        "wrong number of arguments for div. expected: 2, got: %i", argc);
  }

  SValue* lhs = argv;
  SValue* rhs = argv + 1;

  if (lhs->getType() == SType::NIL || rhs->getType() == SType::NIL) {
    *out = SValue();
    return;
  }

  if (!lhs->isConvertibleToNumeric() || !rhs->isConvertibleToNumeric()) {
    RAISE(kRuntimeError, "can't divide %s and %s",
      lhs->getTypeName(),
      rhs->getTypeName());
  }

  SValue lhs_num = lhs->toNumeric();
  SValue rhs_num = rhs->toNumeric();

  *out = SValue((double) (lhs_num.getFloat() / rhs_num.getFloat()));
}

void modExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
  if (argc != 2) {
    RAISE(
        kRuntimeError,
        "wrong number of arguments for mod. expected: 2, got: %i", argc);
  }

  SValue* lhs = argv;
  SValue* rhs = argv + 1;

  if (lhs->getType() == SType::NIL || rhs->getType() == SType::NIL) {
    *out = SValue();
    return;
  }

  if (!lhs->isConvertibleToNumeric() || !rhs->isConvertibleToNumeric()) {
    RAISE(kRuntimeError, "can't modulo %s and %s",
      lhs->getTypeName(),
      rhs->getTypeName());
  }

  SValue lhs_num = lhs->toNumeric();
  SValue rhs_num = rhs->toNumeric();

  if (lhs_num.getType() == SType::INT64 && rhs_num.getType() == SType::INT64) {
    *out = SValue((int64_t) (lhs_num.getInteger() % rhs_num.getInteger()));
  } else {
    *out = SValue((double) fmod(lhs_num.getFloat(), rhs_num.getFloat()));
  }
}

void powExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
  if (argc != 2) {
    RAISE(
        kRuntimeError,
        "wrong number of arguments for pow. expected: 2, got: %i", argc);
  }

  SValue* lhs = argv;
  SValue* rhs = argv + 1;

  if (lhs->getType() == SType::NIL || rhs->getType() == SType::NIL) {
    *out = SValue();
    return;
  }

  if (!lhs->isConvertibleToNumeric() || !rhs->isConvertibleToNumeric()) {
    RAISE(kRuntimeError, "can't modulo %s and %s",
      lhs->getTypeName(),
      rhs->getTypeName());
  }

  SValue lhs_num = lhs->toNumeric();
  SValue rhs_num = rhs->toNumeric();

  if (lhs_num.getType() == SType::INT64 && rhs_num.getType() == SType::INT64) {
    *out = SValue((int64_t) pow(lhs_num.getInteger(), rhs_num.getInteger()));
  } else {
    *out = SValue((double) pow(lhs_num.getFloat(), rhs_num.getFloat()));
  }
}

void roundExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
  switch (argc) {

    // round to integer
    case 1:
      RAISE(kNotYetImplementedError);

    // round to arbitrary precision float
    case 2:
      RAISE(kNotYetImplementedError);

    default:
      RAISE(
          kRuntimeError,
          "wrong number of arguments for ROUND. expected: 1 or 2, got: %i", argc);
  }
}

void truncateExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
  switch (argc) {

    // truncate to integer
    case 1: {
      SValue* val = argv;
      switch(val->getType()) {
        case SType::INT64:
        case SType::FLOAT64:
          *out = SValue(SValue::IntegerType(val->getFloat()));
          return;
        case SType::NIL:
          *out = SValue();
          return;
        default:
          RAISE(kRuntimeError, "can't TRUNCATE %s", val->getTypeName());
      }
    }

    // truncate to specified number of decimal places
    case 2:
      RAISE(kNotYetImplementedError);

    default:
      RAISE(
          kRuntimeError,
          "wrong number of arguments for TRUNCATE. expected: 1 or 2, got: %i", argc);
  }
}


}
}
