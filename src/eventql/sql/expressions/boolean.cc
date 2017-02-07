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
#include <eventql/sql/expressions/boolean.h>
#include <eventql/util/exception.h>

namespace csql {
namespace expressions {

void logical_and_call(sql_txn* ctx, VMStack* stack) {
  auto right = popBool(stack);
  auto left = popBool(stack);
  pushBool(stack, left && right);
}

const SFunction logical_and(
    { SType::BOOL, SType::BOOL },
    SType::BOOL,
    &logical_and_call);


void logical_or_call(sql_txn* ctx, VMStack* stack) {
  auto right = popBool(stack);
  auto left = popBool(stack);
  pushBool(stack, left || right);
}

const SFunction logical_or(
    { SType::BOOL, SType::BOOL },
    SType::BOOL,
    &logical_or_call);


void eq_uint64_call(sql_txn* ctx, VMStack* stack) {
  auto right = popUInt64(stack);
  auto left = popUInt64(stack);
  pushBool(stack, left == right);
}

const SFunction eq_uint64(
    { SType::UINT64, SType::UINT64 },
    SType::BOOL,
    &eq_uint64_call);

const SFunction eq_timestamp64(
    { SType::TIMESTAMP64, SType::TIMESTAMP64 },
    SType::BOOL,
    &eq_uint64_call);


void lt_uint64_call(sql_txn* ctx, VMStack* stack) {
  auto right = popUInt64(stack);
  auto left = popUInt64(stack);
  pushBool(stack, left < right);
}

const SFunction lt_uint64(
    { SType::UINT64, SType::UINT64 },
    SType::BOOL,
    &lt_uint64_call);

const SFunction lt_timestamp64(
    { SType::TIMESTAMP64, SType::TIMESTAMP64 },
    SType::BOOL,
    &lt_uint64_call);


void lte_uint64_call(sql_txn* ctx, VMStack* stack) {
  auto right = popUInt64(stack);
  auto left = popUInt64(stack);
  pushBool(stack, left <= right);
}

const SFunction lte_uint64(
    { SType::UINT64, SType::UINT64 },
    SType::BOOL,
    &lte_uint64_call);

const SFunction lte_timestamp64(
    { SType::TIMESTAMP64, SType::TIMESTAMP64 },
    SType::BOOL,
    &lte_uint64_call);

void gt_uint64_call(sql_txn* ctx, VMStack* stack) {
  auto right = popUInt64(stack);
  auto left = popUInt64(stack);
  pushBool(stack, left > right);
}

const SFunction gt_uint64(
    { SType::UINT64, SType::UINT64 },
    SType::BOOL,
    &gt_uint64_call);

const SFunction gt_timestamp64(
    { SType::TIMESTAMP64, SType::TIMESTAMP64 },
    SType::BOOL,
    &gt_uint64_call);


void gte_uint64_call(sql_txn* ctx, VMStack* stack) {
  auto right = popUInt64(stack);
  auto left = popUInt64(stack);
  pushBool(stack, left >= right);
}

const SFunction gte_uint64(
    { SType::UINT64, SType::UINT64 },
    SType::BOOL,
    &gte_uint64_call);

const SFunction gte_timestamp64(
    { SType::TIMESTAMP64, SType::TIMESTAMP64 },
    SType::BOOL,
    &gte_uint64_call);

//void eqExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
//  if (argc != 2) {
//    RAISE(
//        kRuntimeError,
//        "wrong number of arguments for eq. expected: 2, got: %i", argc);
//  }
//
//  SValue* lhs = argv;
//  SValue* rhs = argv + 1;
//
//  if (lhs->getType() == SType::NIL ^ rhs->getType() == SType::NIL) {
//    *out = SValue::newBool(false);
//    return;
//  }
//
//  if (lhs->isConvertibleToNumeric() && rhs->isConvertibleToNumeric()) {
//    *out = SValue::newBool(lhs->getFloat() == rhs->getFloat());
//    return;
//  }
//
//  if (lhs->getType() == SType::STRING || rhs->getType() == SType::STRING) {
//    *out = SValue::newBool(lhs->getString() == rhs->getString());
//    return;
//  }
//
//  if (lhs->isConvertibleToBool() || rhs->isConvertibleToBool()) {
//    *out = SValue::newBool(lhs->getBool() == rhs->getBool());
//    return;
//  }
//
//  RAISE(kRuntimeError, "can't compare %s with %s",
//      lhs->getTypeName(),
//      rhs->getTypeName());
//}
//
//void neqExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
//  SValue ret;
//  eqExpr(ctx, argc, argv, &ret);
//  *out = SValue(!ret.getValue<bool>());
//}
//
//void andExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
//  if (argc != 2) {
//    RAISE(
//        kRuntimeError,
//        "wrong number of arguments for AND. expected: 2, got: %i", argc);
//  }
//
//  SValue* lhs = argv;
//  SValue* rhs = argv + 1;
//  *out = SValue(lhs->getBool() && rhs->getBool());
//}
//
//void orExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
//  if (argc != 2) {
//    RAISE(
//        kRuntimeError,
//        "wrong number of arguments for or. expected: 2, got: %i", argc);
//  }
//
//  SValue* lhs = argv;
//  SValue* rhs = argv + 1;
//  *out = SValue(lhs->getBool() || rhs->getBool());
//}
//
//void negExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
//  if (argc != 1) {
//    RAISE(
//        kRuntimeError,
//        "wrong number of arguments for neg. expected: 1, got: %i", argc);
//  }
//
//  SValue* val = argv;
//
//  switch(val->getType()) {
//    case SType::INT64:
//      *out = SValue(val->getInteger() * -1);
//      return;
//    case SType::FLOAT64:
//      *out = SValue(val->getFloat() * -1.0f);
//      return;
//    case SType::BOOL:
//      *out = SValue(!val->getBool());
//      return;
//    case SType::NIL:
//      *out = SValue();
//      return;
//    default:
//      break;
//  }
//
//  RAISE(kRuntimeError, "can't negate %s",
//      val->getTypeName());
//}
//
//void ltExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
//  if (argc != 2) {
//    RAISE(
//        kRuntimeError,
//        "wrong number of arguments for ltExpr. expected: 2, got: %i", argc);
//  }
//
//  SValue* lhs = argv;
//  SValue* rhs = argv + 1;
//
//  switch(lhs->getType()) {
//    case SType::INT64:
//    case SType::TIMESTAMP64:
//      switch(rhs->getType()) {
//        case SType::INT64:
//        case SType::TIMESTAMP64:
//          *out = SValue(lhs->getInteger() < rhs->getInteger());
//          return;
//        case SType::FLOAT64:
//          *out = SValue(lhs->getFloat() < rhs->getFloat());
//          return;
//        case SType::NIL:
//          *out = SValue(lhs->getFloat() < 0.0f);
//          return;
//        default:
//          break;
//      }
//      break;
//    case SType::FLOAT64:
//      switch(rhs->getType()) {
//        case SType::INT64:
//        case SType::FLOAT64:
//        case SType::TIMESTAMP64:
//          *out = SValue(lhs->getFloat() < rhs->getFloat());
//          return;
//        case SType::NIL:
//          *out = SValue(lhs->getFloat() < 0.0f);
//          return;
//        default:
//          break;
//      }
//      break;
//    case SType::NIL:
//      switch(rhs->getType()) {
//        case SType::INT64:
//        case SType::FLOAT64:
//        case SType::TIMESTAMP64:
//          *out = SValue(0.0f < rhs->getFloat());
//          return;
//        case SType::NIL:
//          *out = SValue(SValue::BoolType(false));
//          return;
//        default:
//          break;
//      }
//    default:
//      break;
//  }
//
//  if (lhs->getType() == SType::STRING ||
//      rhs->getType() == SType::STRING) {
//    *out = SValue(lhs->getString() < rhs->getString());
//    return;
//  }
//
//  RAISE(kRuntimeError, "can't compare %s with %s",
//      lhs->getTypeName(),
//      rhs->getTypeName());
//}
//
//void lteExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
//  if (argc != 2) {
//    RAISE(
//        kRuntimeError,
//        "wrong number of arguments for lteExpr. expected: 2, got: %i", argc);
//  }
//
//  SValue* lhs = argv;
//  SValue* rhs = argv + 1;
//
//  switch(lhs->getType()) {
//    case SType::INT64:
//    case SType::TIMESTAMP64:
//      switch(rhs->getType()) {
//        case SType::INT64:
//        case SType::TIMESTAMP64:
//          *out = SValue(lhs->getInteger() <= rhs->getInteger());
//          return;
//        case SType::FLOAT64:
//          *out = SValue(lhs->getFloat() <= rhs->getFloat());
//          return;
//        case SType::NIL:
//          *out = SValue(lhs->getFloat() <= 0.0f);
//          return;
//        default:
//          break;
//      }
//      break;
//    case SType::FLOAT64:
//      switch(rhs->getType()) {
//        case SType::INT64:
//        case SType::FLOAT64:
//        case SType::TIMESTAMP64:
//          *out = SValue(lhs->getFloat() <= rhs->getFloat());
//          return;
//        case SType::NIL:
//          *out = SValue(lhs->getFloat() <= 0.0f);
//          return;
//        default:
//          break;
//      }
//      break;
//    case SType::NIL:
//      switch(rhs->getType()) {
//        case SType::INT64:
//        case SType::FLOAT64:
//        case SType::TIMESTAMP64:
//          *out = SValue(0.0f <= rhs->getFloat());
//          return;
//        case SType::NIL:
//          *out = SValue(SValue::BoolType(true));
//          return;
//        default:
//          break;
//      }
//    default:
//      break;
//  }
//
//  if (lhs->getType() == SType::STRING ||
//      rhs->getType() == SType::STRING) {
//    *out = SValue(lhs->getString() <= rhs->getString());
//    return;
//  }
//
//  RAISE(kRuntimeError, "can't compare %s with %s",
//      lhs->getTypeName(),
//      rhs->getTypeName());
//}
//
//void gtExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
//  if (argc != 2) {
//    RAISE(
//        kRuntimeError,
//        "wrong number of arguments for gtExpr. expected: 2, got: %i", argc);
//  }
//
//  SValue* lhs = argv;
//  SValue* rhs = argv + 1;
//
//  switch(lhs->getType()) {
//    case SType::INT64:
//    case SType::TIMESTAMP64:
//      switch(rhs->getType()) {
//        case SType::INT64:
//        case SType::TIMESTAMP64:
//          *out = SValue(lhs->getInteger() > rhs->getInteger());
//          return;
//        case SType::FLOAT64:
//          *out = SValue(lhs->getFloat() > rhs->getFloat());
//          return;
//        case SType::NIL:
//          *out = SValue(lhs->getFloat() > 0.0f);
//          return;
//        default:
//          break;
//      }
//      break;
//    case SType::FLOAT64:
//      switch(rhs->getType()) {
//        case SType::INT64:
//        case SType::FLOAT64:
//        case SType::TIMESTAMP64:
//          *out = SValue(lhs->getFloat() > rhs->getFloat());
//          return;
//        case SType::NIL:
//          *out = SValue(lhs->getFloat() > 0.0f);
//          return;
//        default:
//          break;
//      }
//      break;
//    case SType::NIL:
//      switch(rhs->getType()) {
//        case SType::INT64:
//        case SType::FLOAT64:
//        case SType::TIMESTAMP64:
//          *out = SValue(0.0f > rhs->getFloat());
//          return;
//        case SType::NIL:
//          *out = SValue(SValue::BoolType(false));
//          return;
//        default:
//          break;
//      }
//    default:
//      break;
//  }
//
//  if (lhs->getType() == SType::STRING ||
//      rhs->getType() == SType::STRING) {
//    *out = SValue(lhs->getString() > rhs->getString());
//    return;
//  }
//
//  RAISE(kRuntimeError, "can't compare %s with %s",
//      lhs->getTypeName(),
//      rhs->getTypeName());
//}
//
//void gteExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
//  if (argc != 2) {
//    RAISE(
//        kRuntimeError,
//        "wrong number of arguments for gteExpr. expected: 2, got: %i", argc);
//  }
//
//  SValue* lhs = argv;
//  SValue* rhs = argv + 1;
//
//  switch(lhs->getType()) {
//    case SType::INT64:
//    case SType::TIMESTAMP64:
//      switch(rhs->getType()) {
//        case SType::INT64:
//        case SType::TIMESTAMP64:
//          *out = SValue(lhs->getInteger() >= rhs->getInteger());
//          return;
//        case SType::FLOAT64:
//          *out = SValue(lhs->getFloat() >= rhs->getFloat());
//          return;
//        case SType::NIL:
//          *out = SValue(lhs->getFloat() >= 0.0f);
//          return;
//        default:
//          break;
//      }
//      break;
//    case SType::FLOAT64:
//      switch(rhs->getType()) {
//        case SType::INT64:
//        case SType::FLOAT64:
//        case SType::TIMESTAMP64:
//          *out = SValue(lhs->getFloat() >= rhs->getFloat());
//          return;
//        case SType::NIL:
//          *out = SValue(lhs->getFloat() >= 0.0f);
//          return;
//        default:
//          break;
//      }
//      break;
//    case SType::NIL:
//      switch(rhs->getType()) {
//        case SType::INT64:
//        case SType::FLOAT64:
//        case SType::TIMESTAMP64:
//          *out = SValue(0.0f >= rhs->getFloat());
//          return;
//        case SType::NIL:
//          *out = SValue(SValue::BoolType(true));
//          return;
//        default:
//          break;
//      }
//    default:
//      break;
//  }
//
//  if (lhs->getType() == SType::STRING ||
//      rhs->getType() == SType::STRING) {
//    *out = SValue(lhs->getString() >= rhs->getString());
//    return;
//  }
//
//  RAISE(kRuntimeError, "can't compare %s with %s",
//      lhs->getTypeName(),
//      rhs->getTypeName());
//}
//
//void isNullExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out) {
//  if (argc != 1) {
//    RAISEF(
//        kRuntimeError,
//        "wrong number of arguments for isnull. expected: 1, got: $0", argc);
//  }
//
//  if (argv[0].getType() == SType::NIL) {
//    *out = SValue(SValue::BoolType(true));
//  } else {
//    *out = SValue(SValue::BoolType(false));
//  }
//}

}
}
