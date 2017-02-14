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

extern const SFunction add_uint64;
extern const SFunction add_int64;
extern const SFunction add_float64;

extern const SFunction sub_uint64;
extern const SFunction sub_int64;
extern const SFunction sub_float64;

extern const SFunction mul_uint64;
extern const SFunction mul_int64;
extern const SFunction mul_float64;

extern const SFunction div_uint64;
extern const SFunction div_int64;
extern const SFunction div_float64;

extern const SFunction mod_uint64;
extern const SFunction mod_int64;
extern const SFunction mod_float64;

extern const SFunction pow_uint64;
extern const SFunction pow_int64;
extern const SFunction pow_float64;

} // namespace expressions
} // namespace csql

