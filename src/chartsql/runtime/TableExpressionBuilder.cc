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
#include <chartsql/runtime/Union.h>

using namespace fnord;

namespace csql {

ScopedPtr<TableExpression> TableExpressionBuilder::build(
    RefPtr<TableExpressionNode> node,
    DefaultRuntime* runtime,
    TableProvider* tables) {

  if (dynamic_cast<GroupByNode*>(node.get())) {
    return buildGroupBy(node.asInstanceOf<GroupByNode>(), runtime, tables);
  }

  if (dynamic_cast<UnionNode*>(node.get())) {
    return buildUnion(node.asInstanceOf<UnionNode>(), runtime, tables);
  }

  if (dynamic_cast<SequentialScanNode*>(node.get())) {
    return buildSequentialScan(
        node.asInstanceOf<SequentialScanNode>(),
        runtime,
        tables);
  }

  RAISE(
      kRuntimeError,
      "cannot figure out how to build a table expression for this QTree node");
}

ScopedPtr<TableExpression> TableExpressionBuilder::buildGroupBy(
    RefPtr<GroupByNode> node,
    DefaultRuntime* runtime,
    TableProvider* tables) {
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

ScopedPtr<TableExpression> TableExpressionBuilder::buildSequentialScan(
    RefPtr<SequentialScanNode> node,
    DefaultRuntime* runtime,
    TableProvider* tables) {
  const auto& table_name = node->tableName();

  auto seqscan = tables->buildSequentialScan(node, runtime);
  if (seqscan.isEmpty()) {
    RAISEF(kRuntimeError, "table not found: $0", table_name);
  }

  return std::move(seqscan.get());
}

ScopedPtr<TableExpression> TableExpressionBuilder::buildUnion(
    RefPtr<UnionNode> node,
    DefaultRuntime* runtime,
    TableProvider* tables) {
  Vector<ScopedPtr<TableExpression>> union_tables;

  for (const auto& table_name : node->inputTables()) {
    union_tables.emplace_back(build(table_name, runtime, tables));
  }

  return mkScoped(new Union(std::move(union_tables)));
}

} // namespace csql
