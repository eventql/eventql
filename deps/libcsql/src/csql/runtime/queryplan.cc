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
    Transaction* txn,
    Vector<RefPtr<QueryTreeNode>> qtrees) :
    txn_(txn),
    qtrees_(qtrees) {
  for (const auto& qtree : qtrees_) {
    statement_tasks_.emplace_back(
        qtree.asInstanceOf<TableExpressionNode>()->build(txn, &tasks_));

    statement_columns_.emplace_back(
        qtree.asInstanceOf<TableExpressionNode>()->outputColumns());
  }
}

void QueryPlan::execute() {
  if (!scheduler_) {
    RAISE(kRuntimeError, "QueryPlan has no scheduler");
  }

  auto sched = scheduler_(txn_, &tasks_, &callbacks_);
  sched->execute();
}

void QueryPlan::setScheduler(SchedulerFactory scheduler) {
  scheduler_ = scheduler;
}

void QueryPlan::onOutputRow(size_t stmt_idx, RowSinkFn fn) {
  if (stmt_idx >= qtrees_.size()) {
    RAISE(kIndexError, "invalid statement index");
  }

  for (const auto& task_id : statement_tasks_[stmt_idx]) {
    callbacks_.on_row[task_id].emplace_back(fn);
  }
}

const Vector<String>& QueryPlan::getStatementOutputColumns(size_t stmt_idx) {
  if (stmt_idx >= qtrees_.size()) {
    RAISE(kIndexError, "invalid statement index");
  }

  return statement_columns_[stmt_idx];
}

size_t QueryPlan::numStatements() const {
  return qtrees_.size();
}

Statement* QueryPlan::getStatement(size_t stmt_idx) const {
  RAISE(kNotImplementedError);
}

RefPtr<QueryTreeNode> QueryPlan::getStatementQTree(size_t stmt_idx) const {
  if (stmt_idx >= qtrees_.size()) {
    RAISE(kIndexError, "invalid statement index");
  }

  return qtrees_[stmt_idx];
}

void QueryPlan::storeResults(size_t stmt_idx, ResultList* result_list) {
  result_list->addHeader(getStatementOutputColumns(stmt_idx));

  onOutputRow(
      stmt_idx,
      std::bind(
          &ResultList::addRow,
          result_list,
          std::placeholders::_1,
          std::placeholders::_2));
}

}
