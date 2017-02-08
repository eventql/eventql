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
#pragma once
#include <eventql/sql/svalue.h>
#include <eventql/sql/SFunction.h>
#include <eventql/sql/transaction.h>
#include <eventql/sql/runtime/vm.h>

namespace csql {
namespace expressions {

extern const SFunction logical_and;
extern const SFunction logical_or;

extern const SFunction cmp_uint64;
extern const SFunction cmp_int64;
extern const SFunction cmp_float64;
extern const SFunction cmp_timestamp64;

extern const SFunction eq_uint64;
extern const SFunction eq_int64;
extern const SFunction eq_float64;
extern const SFunction eq_bool;
extern const SFunction eq_timestamp64;

extern const SFunction neq_uint64;
extern const SFunction neq_int64;
extern const SFunction neq_float64;
extern const SFunction neq_bool;
extern const SFunction neq_timestamp64;

extern const SFunction lt_uint64;
extern const SFunction lt_int64;
extern const SFunction lt_float64;
extern const SFunction lt_timestamp64;

extern const SFunction lte_uint64;
extern const SFunction lte_int64;
extern const SFunction lte_float64;
extern const SFunction lte_timestamp64;

extern const SFunction gt_uint64;
extern const SFunction gt_int64;
extern const SFunction gt_float64;
extern const SFunction gt_timestamp64;

extern const SFunction gte_uint64;
extern const SFunction gte_int64;
extern const SFunction gte_float64;
extern const SFunction gte_timestamp64;

} // namespace expressions
} // namespace csql

