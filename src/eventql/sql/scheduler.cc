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
#include <eventql/sql/scheduler.h>
#include <eventql/sql/expressions/table_expression.h>
#include <eventql/sql/qtree/QueryTreeNode.h>
#include <eventql/sql/expressions/table/select.h>
#include <eventql/sql/expressions/table/subquery.h>
#include <eventql/sql/expressions/table/orderby.h>
#include <eventql/sql/expressions/table/show_tables.h>
#include <eventql/sql/expressions/table/limit.h>
#include <eventql/sql/expressions/table/describe_table.h>
#include <eventql/sql/expressions/table/groupby.h>
#include <eventql/sql/expressions/table/nested_loop_join.h>
#include <eventql/sql/qtree/SelectExpressionNode.h>
#include <eventql/sql/qtree/SubqueryNode.h>
#include <eventql/sql/qtree/OrderByNode.h>
#include <eventql/sql/qtree/DescribeTableNode.h>
#include <eventql/sql/qtree/LimitNode.h>
#include <eventql/sql/qtree/GroupByNode.h>
#include <eventql/sql/qtree/JoinNode.h>
#include <eventql/sql/runtime/queryplan.h>

#include "eventql/eventql.h"

namespace csql {

static ScopedPtr<TableExpression> buildExpression(
    Transaction* ctx,
    RefPtr<QueryTreeNode> node);

static ScopedPtr<TableExpression> buildLimit(
    Transaction* ctx,
    RefPtr<LimitNode> node) {
  return mkScoped(
      new LimitExpression(
          node->limit(),
          node->offset(),
          buildExpression(ctx, node->inputTable())));
}

static ScopedPtr<TableExpression> buildSelectExpression(
    Transaction* ctx,
    RefPtr<SelectExpressionNode> node) {
  Vector<ValueExpression> select_expressions;
  for (const auto& slnode : node->selectList()) {
    select_expressions.emplace_back(
        ctx->getCompiler()->buildValueExpression(ctx, slnode->expression()));
  }

  return mkScoped(new SelectExpression(
      ctx,
      std::move(select_expressions)));
};

static ScopedPtr<TableExpression> buildSubquery(
    Transaction* txn,
    RefPtr<SubqueryNode> node) {
  Vector<ValueExpression> select_expressions;
  Option<ValueExpression> where_expr;

  if (!node->whereExpression().isEmpty()) {
    where_expr = std::move(Option<ValueExpression>(
        txn->getCompiler()->buildValueExpression(txn, node->whereExpression().get())));
  }

  for (const auto& slnode : node->selectList()) {
    select_expressions.emplace_back(
        txn->getCompiler()->buildValueExpression(txn, slnode->expression()));
  }

  return mkScoped(new SubqueryExpression(
      txn,
      std::move(select_expressions),
      std::move(where_expr),
      buildExpression(txn, node->subquery())));
}

static ScopedPtr<TableExpression> buildOrderByExpression(
    Transaction* txn,
    RefPtr<OrderByNode> node) {
  Vector<OrderByExpression::SortExpr> sort_exprs;
  for (const auto& ss : node->sortSpecs()) {
    OrderByExpression::SortExpr se;
    se.descending = ss.descending;
    se.expr = txn->getCompiler()->buildValueExpression(txn, ss.expr);
    sort_exprs.emplace_back(std::move(se));
  }

  return mkScoped(
      new OrderByExpression(
          txn,
          std::move(sort_exprs),
          buildExpression(txn, node->inputTable())));
}

static ScopedPtr<TableExpression> buildSequentialScan(
    Transaction* txn,
    RefPtr<SequentialScanNode> node) {
  const auto& table_name = node->tableName();
  auto table_provider = txn->getTableProvider();

  auto seqscan = table_provider->buildSequentialScan(txn, node);
  if (seqscan.isEmpty()) {
    RAISEF(kRuntimeError, "table not found: $0", table_name);
  }

  return std::move(seqscan.get());
}

static ScopedPtr<TableExpression> buildGroupByExpression(
    Transaction* txn,
    RefPtr<GroupByNode> node) {
  Vector<ValueExpression> select_expressions;
  Vector<ValueExpression> group_expressions;

  for (const auto& slnode : node->selectList()) {
    select_expressions.emplace_back(
        txn->getCompiler()->buildValueExpression(
            txn,
            slnode->expression()));
  }

  for (const auto& e : node->groupExpressions()) {
    group_expressions.emplace_back(
        txn->getCompiler()->buildValueExpression(txn, e));
  }

  return mkScoped(
      new GroupByExpression(
          txn,
          std::move(select_expressions),
          std::move(group_expressions),
          buildExpression(txn, node->inputTable())));
}

static ScopedPtr<TableExpression> buildJoinExpression(
    Transaction* ctx,
    RefPtr<JoinNode> node) {
  Vector<String> column_names;
  Vector<ValueExpression> select_expressions;

  for (const auto& slnode : node->selectList()) {
    select_expressions.emplace_back(
        ctx->getCompiler()->buildValueExpression(ctx, slnode->expression()));
  }

  Option<ValueExpression> where_expr;
  if (!node->whereExpression().isEmpty()) {
    where_expr = std::move(Option<ValueExpression>(
        ctx->getCompiler()->buildValueExpression(ctx, node->whereExpression().get())));
  }

  Option<ValueExpression> join_cond_expr;
  if (!node->joinCondition().isEmpty()) {
    join_cond_expr = std::move(Option<ValueExpression>(
        ctx->getCompiler()->buildValueExpression(ctx, node->joinCondition().get())));
  }

  return mkScoped(
      new NestedLoopJoin(
          ctx,
          node->joinType(),
          node->inputColumnMap(),
          std::move(select_expressions),
          std::move(join_cond_expr),
          std::move(where_expr),
          buildExpression(ctx, node->baseTable()),
          buildExpression(ctx, node->joinedTable())));
}

static ScopedPtr<TableExpression> buildExpression(
    Transaction* ctx,
    RefPtr<QueryTreeNode> node) {

  if (dynamic_cast<LimitNode*>(node.get())) {
    return buildLimit(ctx, node.asInstanceOf<LimitNode>());
  }

  if (dynamic_cast<SelectExpressionNode*>(node.get())) {
    return buildSelectExpression(
        ctx,
        node.asInstanceOf<SelectExpressionNode>());
  }

  if (dynamic_cast<SubqueryNode*>(node.get())) {
    return buildSubquery(
        ctx,
        node.asInstanceOf<SubqueryNode>());
  }

  if (dynamic_cast<OrderByNode*>(node.get())) {
    return buildOrderByExpression(ctx, node.asInstanceOf<OrderByNode>());
  }

  if (dynamic_cast<SequentialScanNode*>(node.get())) {
    return buildSequentialScan(
        ctx,
        node.asInstanceOf<SequentialScanNode>());
  }

  if (dynamic_cast<GroupByNode*>(node.get())) {
    return buildGroupByExpression(
        ctx,
        node.asInstanceOf<GroupByNode>());
  }

  if (dynamic_cast<ShowTablesNode*>(node.get())) {
    return mkScoped(new ShowTablesExpression(ctx));
  }

  if (dynamic_cast<DescribeTableNode*>(node.get())) {
    return mkScoped(new DescribeTableStatement(
        ctx,
        node.asInstanceOf<DescribeTableNode>()->tableName()));
  }

  if (dynamic_cast<JoinNode*>(node.get())) {
    return buildJoinExpression(
        ctx,
        node.asInstanceOf<JoinNode>());
  }

  RAISEF(
      kRuntimeError,
      "cannot figure out how to execute that query, sorry. -- $0",
      node->toString());
};

ScopedPtr<ResultCursor> DefaultScheduler::execute(
    QueryPlan* query_plan,
    size_t stmt_idx) {
  return mkScoped(
      new TableExpressionResultCursor(
          buildExpression(
              query_plan->getTransaction(),
              query_plan->getStatement(stmt_idx))));
};


} // namespace csql
