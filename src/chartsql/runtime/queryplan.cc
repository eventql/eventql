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

void QueryPlan::executeStatement(
    size_t stmt_idx,
    Function<bool (int argc, const SValue* argv)> fn) {
  if (stmt_idx >= statements_.size()) {
    RAISE(kIndexError, "invalid statement index");
  }

  auto stmt = statements_[stmt_idx];
  if (dynamic_cast<TableExpressionNode*>(stmt.get())) {
    auto table_expr = runtime_->buildTableExpression(
        stmt.asInstanceOf<TableExpressionNode>(),
        tables_);

    table_expr->execute(&ctx_, fn);
    return;
  }

  RAISE(
      kRuntimeError,
      "cannot figure out how to execute this query plan");
}

//QueryPlan::QueryPlan(TableRepository* table_repo) : table_repo_(table_repo) {}
//
//void QueryPlan::addQuery(std::unique_ptr<QueryPlanNode> query) {
//  queries_.emplace_back(std::move(query));
//}
//
//const std::vector<std::unique_ptr<QueryPlanNode>>& QueryPlan::queries() {
//  return queries_;
//}
//
//TableRepository* QueryPlan::tableRepository() {
//  return table_repo_;
//}

}
