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

extern const SFunction to_nil;

extern const SFunction to_nil_uint64;
extern const SFunction to_nil_int64;
extern const SFunction to_nil_float64;
extern const SFunction to_nil_bool;
extern const SFunction to_nil_string;
extern const SFunction to_nil_timestamp64;

void toStringExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out);
void toIntExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out);
void toFloatExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out);
void toBoolExpr(sql_txn* ctx, int argc, SValue* argv, SValue* out);

}
}
