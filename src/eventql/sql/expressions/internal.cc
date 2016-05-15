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
#include <eventql/sql/expressions/internal.h>
#include <eventql/sql/svalue.h>

namespace csql {
namespace expressions {

/**
 * COUNT() expression
 */
void repeatValueExprAcc(sql_txn* ctx, void* scratchpad, int argc, SValue* argv) {
  *static_cast<SValue*>(scratchpad) = *argv;
}

void repeatValueExprGet(sql_txn* ctx, void* scratchpad, SValue* out) {
  *out = *static_cast<SValue*>(scratchpad);
}

void repeatValueExprInit(sql_txn* ctx, void* scratchpad) {
  new (scratchpad) SValue();
}

void repeatValueExprFree(sql_txn* ctx, void* scratchpad) {
  static_cast<SValue*>(scratchpad)->~SValue();
}

void repeatValueExprReset(sql_txn* ctx, void* scratchpad) {
 // do nothing
}

void repeatValueExprMerge(sql_txn* ctx, void* scratchpad, const void* other) {
  *static_cast<SValue*>(scratchpad) = *static_cast<const SValue*>(other);
}

void repeatValueExprSave(sql_txn* ctx, void* scratchpad, OutputStream* os) {
  static_cast<SValue*>(scratchpad)->encode(os);
}

void repeatValueExprLoad(sql_txn* ctx, void* scratchpad, InputStream* is) {
  static_cast<SValue*>(scratchpad)->decode(is);
}

const AggregateFunction kRepeatValueExpr {
  .scratch_size = sizeof(SValue),
  .accumulate = &repeatValueExprAcc,
  .get = &repeatValueExprGet,
  .reset = &repeatValueExprReset,
  .init = &repeatValueExprInit,
  .free = &repeatValueExprFree,
  .merge = &repeatValueExprMerge,
  .savestate = &repeatValueExprSave,
  .loadstate = &repeatValueExprLoad
};

}
}
