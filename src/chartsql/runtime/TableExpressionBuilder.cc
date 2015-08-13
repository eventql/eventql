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
#include <chartsql/runtime/GroupByMerge.h>
#include <chartsql/runtime/Union.h>
#include <chartsql/runtime/SelectExpression.h>
#include <chartsql/runtime/limitclause.h>
#include <chartsql/runtime/orderby.h>
#include <chartsql/runtime/ShowTablesStatement.h>
#include <chartsql/runtime/DescribeTableStatement.h>

using namespace stx;

namespace csql {

ScopedPtr<TableExpression> TableExpressionBuilder::build(
    RefPtr<QueryTreeNode> node,
    QueryBuilder* runtime,
    TableProvider* tables) {

  if (dynamic_cast<LimitNode*>(node.get())) {
    return buildLimit(node.asInstanceOf<LimitNode>(), runtime, tables);
  }

  if (dynamic_cast<OrderByNode*>(node.get())) {
    return buildOrderBy(node.asInstanceOf<OrderByNode>(), runtime, tables);
  }

  if (dynamic_cast<GroupByNode*>(node.get())) {
    return buildGroupBy(node.asInstanceOf<GroupByNode>(), runtime, tables);
  }

  if (dynamic_cast<GroupByMergeNode*>(node.get())) {
    return buildGroupMerge(
        node.asInstanceOf<GroupByMergeNode>(),
        runtime,
        tables);
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

  if (dynamic_cast<SelectExpressionNode*>(node.get())) {
    return buildSelectExpression(
        node.asInstanceOf<SelectExpressionNode>(),
        runtime,
        tables);
  }

  if (dynamic_cast<ShowTablesNode*>(node.get())) {
    return mkScoped<csql::TableExpression>(new ShowTablesStatement(tables));
  }

  if (dynamic_cast<DescribeTableNode*>(node.get())) {
    return buildDescribeTableStatment(
        node.asInstanceOf<DescribeTableNode>(),
        runtime,
        tables);
  }

  RAISE(
      kRuntimeError,
      "cannot figure out how to build a table expression for this QTree node");
}

ScopedPtr<TableExpression> TableExpressionBuilder::buildGroupBy(
    RefPtr<GroupByNode> node,
    QueryBuilder* runtime,
    TableProvider* tables) {
  Vector<String> column_names;
  Vector<ScopedPtr<ValueExpression>> select_expressions;
  Vector<ScopedPtr<ValueExpression>> group_expressions;

  for (const auto& slnode : node->selectList()) {
    column_names.emplace_back(slnode->columnName());

    select_expressions.emplace_back(
        runtime->buildValueExpression(slnode->expression()));
  }

  for (const auto& e : node->groupExpressions()) {
    group_expressions.emplace_back(runtime->buildValueExpression(e));
  }

  auto next = build(node->inputTable(), runtime, tables);

  return mkScoped(
      new GroupBy(
          std::move(next),
          column_names,
          std::move(select_expressions),
          std::move(group_expressions)));
}

ScopedPtr<TableExpression> TableExpressionBuilder::buildSequentialScan(
    RefPtr<SequentialScanNode> node,
    QueryBuilder* runtime,
    TableProvider* tables) {
  const auto& table_name = node->tableName();

  auto seqscan = tables->buildSequentialScan(node, runtime);
  if (seqscan.isEmpty()) {
    RAISEF(kRuntimeError, "table not found: $0", table_name);
  }

  return std::move(seqscan.get());
}

ScopedPtr<TableExpression> TableExpressionBuilder::buildGroupMerge(
    RefPtr<GroupByMergeNode> node,
    QueryBuilder* runtime,
    TableProvider* tables) {
  Vector<ScopedPtr<GroupByExpression>> shards;

  for (const auto& table : node->inputTables()) {
    auto shard = build(table, runtime, tables);
    auto tshard = dynamic_cast<GroupByExpression*>(shard.get());
    if (tshard) {
      shard.release();
    } else {
      RAISE(kIllegalStateError, "GROUP MERGE subexpression must be a GROUP BY");
    }

    shards.emplace_back(mkScoped(tshard));
  }

  return mkScoped(new GroupByMerge(std::move(shards)));
}

ScopedPtr<TableExpression> TableExpressionBuilder::buildUnion(
    RefPtr<UnionNode> node,
    QueryBuilder* runtime,
    TableProvider* tables) {
  Vector<ScopedPtr<TableExpression>> union_tables;

  for (const auto& table : node->inputTables()) {
    union_tables.emplace_back(build(table, runtime, tables));
  }

  return mkScoped(new Union(std::move(union_tables)));
}

ScopedPtr<TableExpression> TableExpressionBuilder::buildLimit(
    RefPtr<LimitNode> node,
    QueryBuilder* runtime,
    TableProvider* tables) {
  return mkScoped(
      new LimitClause(
          node->limit(),
          node->offset(),
          build(node->inputTable(), runtime, tables)));
}

ScopedPtr<TableExpression> TableExpressionBuilder::buildOrderBy(
    RefPtr<OrderByNode> node,
    QueryBuilder* runtime,
    TableProvider* tables) {
  return mkScoped(
      new OrderBy(
          node->sortSpecs(),
          node->maxOutputColumnIndex(),
          build(node->inputTable(), runtime, tables)));
}

ScopedPtr<TableExpression> TableExpressionBuilder::buildSelectExpression(
    RefPtr<SelectExpressionNode> node,
    QueryBuilder* runtime,
    TableProvider* tables) {
  Vector<String> column_names;
  Vector<ScopedPtr<ValueExpression>> select_expressions;

  for (const auto& slnode : node->selectList()) {
    column_names.emplace_back(slnode->columnName());

    select_expressions.emplace_back(
        runtime->buildValueExpression(slnode->expression()));
  }

  return mkScoped(new SelectExpression(
      column_names,
      std::move(select_expressions)));
}

ScopedPtr<TableExpression> TableExpressionBuilder::buildDescribeTableStatment(
    RefPtr<DescribeTableNode> node,
    QueryBuilder* runtime,
    TableProvider* tables) {
  auto table_info = tables->describe(node->tableName());
  if (table_info.isEmpty()) {
    RAISEF(kNotFoundError, "table not found: $0", node->tableName());
  }

  return mkScoped(new DescribeTableStatement(table_info.get()));
}

} // namespace csql
