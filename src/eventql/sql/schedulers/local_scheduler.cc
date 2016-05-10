/**
 * This file is part of the "libcsql" project
 *   Copyright (c) 2015 Paul Asmuth, zScale Technology GmbH
 *
 * libcsql is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <eventql/sql/schedulers/local_scheduler.h>
#include <eventql/sql/runtime/queryplan.h>

using namespace stx;

namespace csql {

ScopedPtr<ResultCursor> LocalScheduler::execute(
    QueryPlan* query_plan,
    size_t stmt_idx) {
  auto table_expr = buildExpression(
      query_plan->getTransaction(),
      query_plan->getStatement(stmt_idx));

  return mkScoped(new LocalResultCursor(std::move(table_expr)));
};

ScopedPtr<TableExpression> LocalScheduler::buildExpression(
    Transaction* ctx,
    RefPtr<QueryTreeNode> node) {

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

  if (dynamic_cast<ShowTablesNode*>(node.get())) {
    return mkScoped(new ShowTablesExpression(ctx));
  }

  RAISEF(
      kRuntimeError,
      "cannot figure out how to execute that query, sorry. -- $0",
      node->toString());
};

ScopedPtr<TableExpression> LocalScheduler::buildSelectExpression(
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

ScopedPtr<TableExpression> LocalScheduler::buildSubquery(
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

ScopedPtr<TableExpression> LocalScheduler::buildOrderByExpression(
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

ScopedPtr<TableExpression> LocalScheduler::buildSequentialScan(
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

LocalResultCursor::LocalResultCursor(
    ScopedPtr<TableExpression> table_expression) :
    table_expression_(std::move(table_expression)),
    cursor_(table_expression_->execute()) {}

bool LocalResultCursor::next(SValue* row, int row_len) {
  return cursor_->next(row, row_len);
}

size_t LocalResultCursor::getNumColumns() {
  return cursor_->getNumColumns();
}

} // namespace csql
