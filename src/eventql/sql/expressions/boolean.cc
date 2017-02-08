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


/**
 * logical_and(bool, bool) -> bool
 */
void logical_and_call(sql_txn* ctx, VMStack* stack) {
  auto right = popBool(stack);
  auto left = popBool(stack);
  pushBool(stack, left && right);
}

const SFunction logical_and(
    { SType::BOOL, SType::BOOL },
    SType::BOOL,
    &logical_and_call);


/**
 * logical_or(bool, bool) -> bool
 */
void logical_or_call(sql_txn* ctx, VMStack* stack) {
  auto right = popBool(stack);
  auto left = popBool(stack);
  pushBool(stack, left || right);
}

const SFunction logical_or(
    { SType::BOOL, SType::BOOL },
    SType::BOOL,
    &logical_or_call);


/**
 * cmp(uint64, uint64) -> int64
 * cmp(timestamp64, timestamp64) -> int64
 */
void cmp_uint64_call(sql_txn* ctx, VMStack* stack) {
  auto right = popUInt64(stack);
  auto left = popUInt64(stack);
  if (left < right) {
    pushInt64(stack, -1);
  } else if (left > right) {
    pushInt64(stack, 1);
  } else {
    pushInt64(stack, 0);
  }
}

const SFunction cmp_uint64(
    { SType::UINT64, SType::UINT64 },
    SType::INT64,
    &cmp_uint64_call);

const SFunction cmp_timestamp64(
    { SType::TIMESTAMP64, SType::TIMESTAMP64 },
    SType::INT64,
    &cmp_uint64_call);


/**
 * eq(uint64, uint64) -> bool
 * eq(timestamp64, timestamp64) -> bool
 */
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


/**
 * eq(int64, int64) -> bool
 */
void eq_int64_call(sql_txn* ctx, VMStack* stack) {
  auto right = popInt64(stack);
  auto left = popInt64(stack);
  pushBool(stack, left == right);
}

const SFunction eq_int64(
    { SType::INT64, SType::INT64 },
    SType::BOOL,
    &eq_int64_call);


/**
 * eq(float64, float64) -> bool
 */
void eq_float64_call(sql_txn* ctx, VMStack* stack) {
  auto right = popFloat64(stack);
  auto left = popFloat64(stack);
  pushBool(stack, left == right);
}

const SFunction eq_float64(
    { SType::FLOAT64, SType::FLOAT64 },
    SType::BOOL,
    &eq_float64_call);


/**
 * lt(uint64, uint64) -> bool
 * lt(timestamp64, timestamp4) -> bool
 */
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


/**
 * lte(uint64, uint64) -> bool
 * lte(timestamp64, timestamp64) -> bool
 */
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


/**
 * gt(uint64, uint64) -> bool
 * gt(timestamp64, timestamp4) -> bool
 */
const SFunction gt_uint64(
    { SType::UINT64, SType::UINT64 },
    SType::BOOL,
    &gt_uint64_call);

const SFunction gt_timestamp64(
    { SType::TIMESTAMP64, SType::TIMESTAMP64 },
    SType::BOOL,
    &gt_uint64_call);


/**
 * gte(uint64, uint64) -> bool
 * gte(timestamp64, timestamp64) -> bool
 */
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


} // namespace expressions
} // namespace csql

