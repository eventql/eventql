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
#include <csql/expressions/internal.h>
#include <csql/svalue.h>

namespace csql {
namespace expressions {

/**
 * COUNT() expression
 */
void repeatValueExprAcc(void* scratchpad, int argc, SValue* argv) {
  *static_cast<SValue*>(scratchpad) = *argv;
}

void repeatValueExprGet(void* scratchpad, SValue* out) {
  *out = *static_cast<SValue*>(scratchpad);
}

void repeatValueExprInit(void* scratchpad) {
  new (scratchpad) SValue();
}

void repeatValueExprFree(void* scratchpad) {
  static_cast<SValue*>(scratchpad)->~SValue();
}

void repeatValueExprReset(void* scratchpad) {
 // do nothing
}

void repeatValueExprMerge(void* scratchpad, const void* other) {
  *static_cast<SValue*>(scratchpad) = *static_cast<const SValue*>(other);
}

void repeatValueExprSave(void* scratchpad, OutputStream* os) {
  static_cast<SValue*>(scratchpad)->encode(os);
}

void repeatValueExprLoad(void* scratchpad, InputStream* is) {
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
