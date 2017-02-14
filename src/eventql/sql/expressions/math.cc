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

void add_uint64_call(sql_txn* ctx, VMStack* stack) {
  auto right = popUInt64(stack);
  auto left = popUInt64(stack);
  pushUInt64(stack, left + right);
}

const SFunction add_uint64(
    { SType::UINT64, SType::UINT64 },
    SType::UINT64,
    &add_uint64_call);

void add_int64_call(sql_txn* ctx, VMStack* stack) {
  auto right = popInt64(stack);
  auto left = popInt64(stack);
  pushInt64(stack, left + right);
}

const SFunction add_int64(
    { SType::INT64, SType::INT64 },
    SType::INT64,
    &add_int64_call);

void add_float64_call(sql_txn* ctx, VMStack* stack) {
  auto right = popFloat64(stack);
  auto left = popFloat64(stack);
  pushFloat64(stack, left + right);
}

const SFunction add_float64(
    { SType::FLOAT64, SType::FLOAT64 },
    SType::FLOAT64,
    &add_float64_call);


void sub_uint64_call(sql_txn* ctx, VMStack* stack) {
  auto right = popUInt64(stack);
  auto left = popUInt64(stack);
  pushUInt64(stack, left - right);
}

const SFunction sub_uint64(
    { SType::UINT64, SType::UINT64 },
    SType::UINT64,
    &sub_uint64_call);

void sub_int64_call(sql_txn* ctx, VMStack* stack) {
  auto right = popInt64(stack);
  auto left = popInt64(stack);
  pushInt64(stack, left - right);
}

const SFunction sub_int64(
    { SType::INT64, SType::INT64 },
    SType::INT64,
    &sub_int64_call);

void sub_float64_call(sql_txn* ctx, VMStack* stack) {
  auto right = popFloat64(stack);
  auto left = popFloat64(stack);
  pushFloat64(stack, left - right);
}

const SFunction sub_float64(
    { SType::FLOAT64, SType::FLOAT64 },
    SType::FLOAT64,
    &sub_float64_call);


void mul_uint64_call(sql_txn* ctx, VMStack* stack) {
  auto right = popUInt64(stack);
  auto left = popUInt64(stack);
  pushUInt64(stack, left * right);
}

const SFunction mul_uint64(
    { SType::UINT64, SType::UINT64 },
    SType::UINT64,
    &mul_uint64_call);

void mul_int64_call(sql_txn* ctx, VMStack* stack) {
  auto right = popInt64(stack);
  auto left = popInt64(stack);
  pushInt64(stack, left * right);
}

const SFunction mul_int64(
    { SType::INT64, SType::INT64 },
    SType::INT64,
    &mul_int64_call);

void mul_float64_call(sql_txn* ctx, VMStack* stack) {
  auto right = popFloat64(stack);
  auto left = popFloat64(stack);
  pushFloat64(stack, left * right);
}

const SFunction mul_float64(
    { SType::FLOAT64, SType::FLOAT64 },
    SType::FLOAT64,
    &mul_float64_call);


void div_uint64_call(sql_txn* ctx, VMStack* stack) {
  auto right = popUInt64(stack);
  auto left = popUInt64(stack);
  if (right == 0) {
    RAISE(kRuntimeError, "division by zero");
  }

  pushUInt64(stack, left / right);
}

const SFunction div_uint64(
    { SType::UINT64, SType::UINT64 },
    SType::UINT64,
    &div_uint64_call);

void div_int64_call(sql_txn* ctx, VMStack* stack) {
  auto right = popInt64(stack);
  auto left = popInt64(stack);
  if (right == 0) {
    RAISE(kRuntimeError, "division by zero");
  }

  pushInt64(stack, left / right);
}

const SFunction div_int64(
    { SType::INT64, SType::INT64 },
    SType::INT64,
    &div_int64_call);

void div_float64_call(sql_txn* ctx, VMStack* stack) {
  auto right = popFloat64(stack);
  auto left = popFloat64(stack);
  pushFloat64(stack, left / right); // division by zero permitted for FLOAT64
}

const SFunction div_float64(
    { SType::FLOAT64, SType::FLOAT64 },
    SType::FLOAT64,
    &div_float64_call);


void mod_uint64_call(sql_txn* ctx, VMStack* stack) {
  auto right = popUInt64(stack);
  auto left = popUInt64(stack);
  if (right == 0) {
    RAISE(kRuntimeError, "modulo by zero");
  }

  pushUInt64(stack, left % right);
}

const SFunction mod_uint64(
    { SType::UINT64, SType::UINT64 },
    SType::UINT64,
    &mod_uint64_call);

void mod_int64_call(sql_txn* ctx, VMStack* stack) {
  auto right = popInt64(stack);
  auto left = popInt64(stack);
  if (right == 0) {
    RAISE(kRuntimeError, "modulo by zero");
  }

  pushInt64(stack, left % right);
}

const SFunction mod_int64(
    { SType::INT64, SType::INT64 },
    SType::INT64,
    &mod_int64_call);

void mod_float64_call(sql_txn* ctx, VMStack* stack) {
  auto right = popFloat64(stack);
  auto left = popFloat64(stack);
  pushFloat64(stack, fmod(left, right)); // modulo by zero permitted for FLOAT64
}

const SFunction mod_float64(
    { SType::FLOAT64, SType::FLOAT64 },
    SType::FLOAT64,
    &mod_float64_call);


void pow_uint64_call(sql_txn* ctx, VMStack* stack) {
  auto right = popUInt64(stack);
  auto left = popUInt64(stack);
  pushUInt64(stack, pow(left, right));
}

const SFunction pow_uint64(
    { SType::UINT64, SType::UINT64 },
    SType::UINT64,
    &pow_uint64_call);

void pow_int64_call(sql_txn* ctx, VMStack* stack) {
  auto right = popInt64(stack);
  auto left = popInt64(stack);
  pushInt64(stack, pow(left, right));
}

const SFunction pow_int64(
    { SType::INT64, SType::INT64 },
    SType::INT64,
    &pow_int64_call);

void pow_float64_call(sql_txn* ctx, VMStack* stack) {
  auto right = popFloat64(stack);
  auto left = popFloat64(stack);
  pushFloat64(stack, pow(left, right));
}

const SFunction pow_float64(
    { SType::FLOAT64, SType::FLOAT64 },
    SType::FLOAT64,
    &pow_float64_call);


} // namespace expressions
} // namespace csql

