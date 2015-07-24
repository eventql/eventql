/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2011-2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <chartsql/parser/astnode.h>
#include <chartsql/runtime/compile.h>
#include <chartsql/svalue.h>
#include <stx/exception.h>

namespace csql {

bool executeExpression(
    Instruction* expr,
    void* scratchpad,
    int row_len,
    const SValue* row,
    int* outc,
    SValue* outv) {
  RAISE(kNotImplementedError);
}

SValue executeSimpleConstExpression(ValueExpressionBuilder* compiler, ASTNode* expr) {
  RAISE(kNotImplementedError);
}

}
