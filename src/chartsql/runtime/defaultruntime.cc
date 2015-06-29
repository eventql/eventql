/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/runtime/defaultruntime.h>

namespace csql {

DefaultRuntime::DefaultRuntime(
    SymbolTable* symbol_table) :
    scalar_exp_builder_(symbol_table) {}

RefPtr<ExecutionPlan> DefaultRuntime::buildExecutionPlan(
    RefPtr<QueryTreeNode> qtree,
    TableRepository* tables) {
  if (dynamic_cast<TableExpressionNode*>(qtree.get())) {
    auto table_expr = table_exp_builder_.build(
        qtree.asInstanceOf<TableExpressionNode>(),
        this,
        tables);

    return new DefaultExecutionPlan(std::move(table_expr));
  }

  RAISE(
      kRuntimeError,
      "cannot figure out how to build a query plan for this QTree node");
}

ScopedPtr<ValueExpression> DefaultRuntime::buildValueExpression(
    RefPtr<ValueExpressionNode> node) {
  return scalar_exp_builder_.compile(node);
}

ScopedPtr<TableExpression> DefaultRuntime::buildTableExpression(
    RefPtr<TableExpressionNode> node,
    TableRepository* tables) {
  return table_exp_builder_.build(node, this, tables);
}

}
