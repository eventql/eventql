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

  RAISE(
      kRuntimeError,
      "cannot figure out how to execute that query, sorry.");
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
    Transaction* ctx,
    RefPtr<SubqueryNode> node) {
  Vector<ValueExpression> select_expressions;
  Option<ValueExpression> where_expr;

  if (!node->whereExpression().isEmpty()) {
    where_expr = std::move(Option<ValueExpression>(
        ctx->getCompiler()->buildValueExpression(ctx, node->whereExpression().get())));
  }

  for (const auto& slnode : node->selectList()) {
    select_expressions.emplace_back(
        ctx->getCompiler()->buildValueExpression(ctx, slnode->expression()));
  }

  return mkScoped(new SubqueryExpression(
      ctx,
      std::move(select_expressions),
      std::move(where_expr),
      buildExpression(ctx, node->subquery())));
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
