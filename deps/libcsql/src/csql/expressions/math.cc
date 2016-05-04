/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include <csql/expressions/math.h>
#include <csql/Transaction.h>

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

  if (lhs->getType() == SQL_NULL || rhs->getType() == SQL_NULL) {
    *out = SValue();
    return;
  }

  if (!lhs->isConvertibleToNumeric() || !rhs->isConvertibleToNumeric()) {
    *out = SValue(lhs->getString() + rhs->getString());
    return;
  }

  SValue lhs_num = lhs->toNumeric();
  SValue rhs_num = rhs->toNumeric();

  if (lhs_num.getType() == SQL_INTEGER && rhs_num.getType() == SQL_INTEGER) {
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

  if (lhs->getType() == SQL_NULL || rhs->getType() == SQL_NULL) {
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

  if (lhs_num.getType() == SQL_INTEGER && rhs_num.getType() == SQL_INTEGER) {
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

  if (lhs->getType() == SQL_NULL || rhs->getType() == SQL_NULL) {
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

  if (lhs_num.getType() == SQL_INTEGER && rhs_num.getType() == SQL_INTEGER) {
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

  if (lhs->getType() == SQL_NULL || rhs->getType() == SQL_NULL) {
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

  if (lhs->getType() == SQL_NULL || rhs->getType() == SQL_NULL) {
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

  if (lhs_num.getType() == SQL_INTEGER && rhs_num.getType() == SQL_INTEGER) {
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

  if (lhs->getType() == SQL_NULL || rhs->getType() == SQL_NULL) {
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

  if (lhs_num.getType() == SQL_INTEGER && rhs_num.getType() == SQL_INTEGER) {
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
        case SQL_INTEGER:
        case SQL_FLOAT:
          *out = SValue(SValue::IntegerType(val->getFloat()));
          return;
        case SQL_NULL:
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
