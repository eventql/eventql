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
#include <assert.h>
#include <math.h>
#include <string.h>
#include <eventql/sql/expressions/boolean.h>
#include <eventql/util/exception.h>

namespace csql {
namespace expressions {

void eqExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
  if (argc != 2) {
    RAISE(
        kRuntimeError,
        "wrong number of arguments for eq. expected: 2, got: %i", argc);
  }

  SValue* lhs = argv;
  SValue* rhs = argv + 1;

  if (lhs->getType() == SQL_NULL ^ rhs->getType() == SQL_NULL) {
    *out = SValue::newBool(false);
    return;
  }

  if (lhs->isConvertibleToNumeric() && rhs->isConvertibleToNumeric()) {
    *out = SValue::newBool(lhs->getFloat() == rhs->getFloat());
    return;
  }

  if (lhs->getType() == SQL_STRING || rhs->getType() == SQL_STRING) {
    *out = SValue::newBool(lhs->getString() == rhs->getString());
    return;
  }

  if (lhs->isConvertibleToBool() || rhs->isConvertibleToBool()) {
    *out = SValue::newBool(lhs->getBool() == rhs->getBool());
    return;
  }

  RAISE(kRuntimeError, "can't compare %s with %s",
      lhs->getTypeName(),
      rhs->getTypeName());
}

void neqExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
  SValue ret;
  eqExpr(ctx, argc, argv, &ret);
  *out = SValue(!ret.getValue<bool>());
}

void andExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
  if (argc != 2) {
    RAISE(
        kRuntimeError,
        "wrong number of arguments for AND. expected: 2, got: %i", argc);
  }

  SValue* lhs = argv;
  SValue* rhs = argv + 1;
  *out = SValue(lhs->getBool() && rhs->getBool());
}

void orExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
  if (argc != 2) {
    RAISE(
        kRuntimeError,
        "wrong number of arguments for or. expected: 2, got: %i", argc);
  }

  SValue* lhs = argv;
  SValue* rhs = argv + 1;
  *out = SValue(lhs->getBool() || rhs->getBool());
}

void negExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
  if (argc != 1) {
    RAISE(
        kRuntimeError,
        "wrong number of arguments for neg. expected: 1, got: %i", argc);
  }

  SValue* val = argv;

  switch(val->getType()) {
    case SQL_INTEGER:
      *out = SValue(val->getInteger() * -1);
      return;
    case SQL_FLOAT:
      *out = SValue(val->getFloat() * -1.0f);
      return;
    case SQL_BOOL:
      *out = SValue(!val->getBool());
      return;
    case SQL_NULL:
      *out = SValue();
      return;
    default:
      break;
  }

  RAISE(kRuntimeError, "can't negate %s",
      val->getTypeName());
}

void ltExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
  if (argc != 2) {
    RAISE(
        kRuntimeError,
        "wrong number of arguments for ltExpr. expected: 2, got: %i", argc);
  }

  SValue* lhs = argv;
  SValue* rhs = argv + 1;

  switch(lhs->getType()) {
    case SQL_INTEGER:
    case SQL_TIMESTAMP:
      switch(rhs->getType()) {
        case SQL_INTEGER:
        case SQL_TIMESTAMP:
          *out = SValue(lhs->getInteger() < rhs->getInteger());
          return;
        case SQL_FLOAT:
          *out = SValue(lhs->getFloat() < rhs->getFloat());
          return;
        case SQL_NULL:
          *out = SValue(lhs->getFloat() < 0.0f);
          return;
        default:
          break;
      }
      break;
    case SQL_FLOAT:
      switch(rhs->getType()) {
        case SQL_INTEGER:
        case SQL_FLOAT:
        case SQL_TIMESTAMP:
          *out = SValue(lhs->getFloat() < rhs->getFloat());
          return;
        case SQL_NULL:
          *out = SValue(lhs->getFloat() < 0.0f);
          return;
        default:
          break;
      }
      break;
    case SQL_NULL:
      switch(rhs->getType()) {
        case SQL_INTEGER:
        case SQL_FLOAT:
        case SQL_TIMESTAMP:
          *out = SValue(0.0f < rhs->getFloat());
          return;
        case SQL_NULL:
          *out = SValue(SValue::BoolType(false));
          return;
        default:
          break;
      }
    default:
      break;
  }

  if (lhs->getType() == SQL_STRING ||
      rhs->getType() == SQL_STRING) {
    *out = SValue(lhs->getString() < rhs->getString());
    return;
  }

  RAISE(kRuntimeError, "can't compare %s with %s",
      lhs->getTypeName(),
      rhs->getTypeName());
}

void lteExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
  if (argc != 2) {
    RAISE(
        kRuntimeError,
        "wrong number of arguments for lteExpr. expected: 2, got: %i", argc);
  }

  SValue* lhs = argv;
  SValue* rhs = argv + 1;

  switch(lhs->getType()) {
    case SQL_INTEGER:
    case SQL_TIMESTAMP:
      switch(rhs->getType()) {
        case SQL_INTEGER:
        case SQL_TIMESTAMP:
          *out = SValue(lhs->getInteger() <= rhs->getInteger());
          return;
        case SQL_FLOAT:
          *out = SValue(lhs->getFloat() <= rhs->getFloat());
          return;
        case SQL_NULL:
          *out = SValue(lhs->getFloat() <= 0.0f);
          return;
        default:
          break;
      }
      break;
    case SQL_FLOAT:
      switch(rhs->getType()) {
        case SQL_INTEGER:
        case SQL_FLOAT:
        case SQL_TIMESTAMP:
          *out = SValue(lhs->getFloat() <= rhs->getFloat());
          return;
        case SQL_NULL:
          *out = SValue(lhs->getFloat() <= 0.0f);
          return;
        default:
          break;
      }
      break;
    case SQL_NULL:
      switch(rhs->getType()) {
        case SQL_INTEGER:
        case SQL_FLOAT:
        case SQL_TIMESTAMP:
          *out = SValue(0.0f <= rhs->getFloat());
          return;
        case SQL_NULL:
          *out = SValue(SValue::BoolType(true));
          return;
        default:
          break;
      }
    default:
      break;
  }

  if (lhs->getType() == SQL_STRING ||
      rhs->getType() == SQL_STRING) {
    *out = SValue(lhs->getString() <= rhs->getString());
    return;
  }

  RAISE(kRuntimeError, "can't compare %s with %s",
      lhs->getTypeName(),
      rhs->getTypeName());
}

void gtExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
  if (argc != 2) {
    RAISE(
        kRuntimeError,
        "wrong number of arguments for gtExpr. expected: 2, got: %i", argc);
  }

  SValue* lhs = argv;
  SValue* rhs = argv + 1;

  switch(lhs->getType()) {
    case SQL_INTEGER:
    case SQL_TIMESTAMP:
      switch(rhs->getType()) {
        case SQL_INTEGER:
        case SQL_TIMESTAMP:
          *out = SValue(lhs->getInteger() > rhs->getInteger());
          return;
        case SQL_FLOAT:
          *out = SValue(lhs->getFloat() > rhs->getFloat());
          return;
        case SQL_NULL:
          *out = SValue(lhs->getFloat() > 0.0f);
          return;
        default:
          break;
      }
      break;
    case SQL_FLOAT:
      switch(rhs->getType()) {
        case SQL_INTEGER:
        case SQL_FLOAT:
        case SQL_TIMESTAMP:
          *out = SValue(lhs->getFloat() > rhs->getFloat());
          return;
        case SQL_NULL:
          *out = SValue(lhs->getFloat() > 0.0f);
          return;
        default:
          break;
      }
      break;
    case SQL_NULL:
      switch(rhs->getType()) {
        case SQL_INTEGER:
        case SQL_FLOAT:
        case SQL_TIMESTAMP:
          *out = SValue(0.0f > rhs->getFloat());
          return;
        case SQL_NULL:
          *out = SValue(SValue::BoolType(false));
          return;
        default:
          break;
      }
    default:
      break;
  }

  if (lhs->getType() == SQL_STRING ||
      rhs->getType() == SQL_STRING) {
    *out = SValue(lhs->getString() > rhs->getString());
    return;
  }

  RAISE(kRuntimeError, "can't compare %s with %s",
      lhs->getTypeName(),
      rhs->getTypeName());
}

void gteExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
  if (argc != 2) {
    RAISE(
        kRuntimeError,
        "wrong number of arguments for gteExpr. expected: 2, got: %i", argc);
  }

  SValue* lhs = argv;
  SValue* rhs = argv + 1;

  switch(lhs->getType()) {
    case SQL_INTEGER:
    case SQL_TIMESTAMP:
      switch(rhs->getType()) {
        case SQL_INTEGER:
        case SQL_TIMESTAMP:
          *out = SValue(lhs->getInteger() >= rhs->getInteger());
          return;
        case SQL_FLOAT:
          *out = SValue(lhs->getFloat() >= rhs->getFloat());
          return;
        case SQL_NULL:
          *out = SValue(lhs->getFloat() >= 0.0f);
          return;
        default:
          break;
      }
      break;
    case SQL_FLOAT:
      switch(rhs->getType()) {
        case SQL_INTEGER:
        case SQL_FLOAT:
        case SQL_TIMESTAMP:
          *out = SValue(lhs->getFloat() >= rhs->getFloat());
          return;
        case SQL_NULL:
          *out = SValue(lhs->getFloat() >= 0.0f);
          return;
        default:
          break;
      }
      break;
    case SQL_NULL:
      switch(rhs->getType()) {
        case SQL_INTEGER:
        case SQL_FLOAT:
        case SQL_TIMESTAMP:
          *out = SValue(0.0f >= rhs->getFloat());
          return;
        case SQL_NULL:
          *out = SValue(SValue::BoolType(true));
          return;
        default:
          break;
      }
    default:
      break;
  }

  if (lhs->getType() == SQL_STRING ||
      rhs->getType() == SQL_STRING) {
    *out = SValue(lhs->getString() >= rhs->getString());
    return;
  }

  RAISE(kRuntimeError, "can't compare %s with %s",
      lhs->getTypeName(),
      rhs->getTypeName());
}

void isNullExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
  if (argc != 1) {
    RAISEF(
        kRuntimeError,
        "wrong number of arguments for isnull. expected: 1, got: $0", argc);
  }

  if (argv[0].getType() == SQL_NULL) {
    *out = SValue(SValue::BoolType(true));
  } else {
    *out = SValue(SValue::BoolType(false));
  }
}

}
}
