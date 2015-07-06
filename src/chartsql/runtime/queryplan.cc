/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/runtime/queryplan.h>
#include <chartsql/runtime/runtime.h>

namespace csql {

QueryPlan::QueryPlan(
    Vector<RefPtr<QueryTreeNode>> statements,
    RefPtr<TableProvider> tables,
    Runtime* runtime) :
    statements_(statements),
    tables_(tables),
    runtime_(runtime) {}

size_t QueryPlan::numStatements() const {
  return statements_.size();
}

ScopedPtr<Statement> QueryPlan::buildStatement(size_t stmt_idx) const {
  if (stmt_idx >= statements_.size()) {
    RAISE(kIndexError, "invalid statement index");
  }

  auto stmt = statements_[stmt_idx];
  if (dynamic_cast<TableExpressionNode*>(stmt.get())) {
    return runtime_->buildTableExpression(
        stmt.asInstanceOf<TableExpressionNode>(),
        tables_);
  }

  RAISE(
      kRuntimeError,
      "cannot figure out how to execute this query plan");
}

}
