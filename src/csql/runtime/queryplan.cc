/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/runtime/queryplan.h>
#include <csql/runtime/runtime.h>

namespace csql {

QueryPlan::QueryPlan(
    Vector<RefPtr<QueryTreeNode>> statements,
    RefPtr<TableProvider> tables,
    QueryBuilder* qbuilder,
    Runtime* runtime) :
    qtrees_(statements),
    tables_(tables),
    runtime_(runtime),
    qbuilder_(qbuilder) {

  for (const auto& stmt : qtrees_) {
    if (dynamic_cast<TableExpressionNode*>(stmt.get())) {
      statements_.emplace_back(qbuilder_->buildTableExpression(
          stmt.asInstanceOf<TableExpressionNode>(),
          tables_,
          runtime_));

      continue;
    }

    if (dynamic_cast<ChartStatementNode*>(stmt.get())) {
      statements_.emplace_back(qbuilder_->buildChartStatement(
          stmt.asInstanceOf<ChartStatementNode>(),
          tables_,
          runtime_));

      continue;
    }

    RAISE(
        kRuntimeError,
        "cannot figure out how to execute this query plan");

  }
}

size_t QueryPlan::numStatements() const {
  return qtrees_.size();
}

Statement* QueryPlan::getStatement(size_t stmt_idx) const {
  if (stmt_idx >= qtrees_.size()) {
    RAISE(kIndexError, "invalid statement index");
  }

  return statements_[stmt_idx].get();
}

RefPtr<QueryTreeNode> QueryPlan::getStatementQTree(size_t stmt_idx) const {
  if (stmt_idx >= qtrees_.size()) {
    RAISE(kIndexError, "invalid statement index");
  }

  return qtrees_[stmt_idx];
}

}
