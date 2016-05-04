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
#include <csql/expressions/boolean.h>
#include <stx/exception.h>

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

  switch(lhs->getType()) {
    case SQL_INTEGER:
    case SQL_TIMESTAMP:
      switch(rhs->getType()) {
        case SQL_INTEGER:
        case SQL_TIMESTAMP:
          *out = SValue(lhs->getInteger() == rhs->getInteger());
          return;
        case SQL_FLOAT:
          *out = SValue(lhs->getFloat() == rhs->getFloat());
          return;
        case SQL_NULL:
          *out = SValue(SValue::BoolType(false));
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
          *out = SValue(lhs->getFloat() == rhs->getFloat());
          return;
        case SQL_NULL:
          *out = SValue(SValue::BoolType(false));
          return;
        default:
          break;
      }
      break;
    case SQL_NULL:
      switch(rhs->getType()) {
        case SQL_NULL:
          *out = SValue(SValue::BoolType(true));
          return;
        default:
          *out = SValue(SValue::BoolType(false));
          return;
      }
      return;
    case SQL_BOOL:
      switch(rhs->getType()) {
        case SQL_BOOL:
          *out = SValue(SValue::BoolType(lhs->getBool() == rhs->getBool()));
          return;
        default:
          break;
      }
      return;
    default:
      break;
  }

  if (lhs->getType() == SQL_STRING ||
      rhs->getType() == SQL_STRING) {
    *out = SValue(lhs->getString() == rhs->getString());
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
