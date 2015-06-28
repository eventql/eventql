/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/runtime/TableExpressionBuilder.h>
#include <chartsql/runtime/groupby.h>

using namespace fnord;

namespace csql {

ScopedPtr<TableExpression> TableExpressionBuilder::build(
    RefPtr<TableExpressionNode> node,
    DefaultRuntime* runtime,
    TableRepository* tables) {

  if (dynamic_cast<GroupByNode*>(node.get())) {
    return buildGroupBy(node.asInstanceOf<GroupByNode>(), runtime, tables);
  }

  RAISE(
      kRuntimeError,
      "cannot figure out how to build a table expression for this QTree node");
}

ScopedPtr<TableExpression> TableExpressionBuilder::buildGroupBy(
    RefPtr<GroupByNode> node,
    DefaultRuntime* runtime,
    TableRepository* tables) {
  Vector<ScopedPtr<ScalarExpression>> select_expressions;
  Vector<ScopedPtr<ScalarExpression>> group_expressions;

  for (const auto& slnode : node->selectList()) {
    select_expressions.emplace_back(
        runtime->buildScalarExpression(slnode->expression()));
  }

  for (const auto& e : node->groupExpressions()) {
    group_expressions.emplace_back(runtime->buildScalarExpression(e));
  }

  auto next = build(node->inputTable(), runtime, tables);

  return mkScoped(
      new GroupBy(
          std::move(next),
          std::move(select_expressions),
          std::move(group_expressions)));
}


} // namespace csql
