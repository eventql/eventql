/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <chartsql/qtree/ValueExpressionNode.h>
#include <chartsql/runtime/symboltable.h>
#include <chartsql/runtime/ValueExpression.h>

using namespace stx;

namespace csql {
class ASTNode;

class ValueExpressionBuilder : public RefCounted {
public:

  ValueExpressionBuilder(SymbolTable* symbol_table);

  ValueExpression compile(RefPtr<ValueExpressionNode> node);

  SymbolTable* symbolTable() { return symbol_table_; }

protected:
  SymbolTable* symbol_table_;
};
} // namespace csql
