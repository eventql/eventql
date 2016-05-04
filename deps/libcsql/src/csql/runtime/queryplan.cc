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

ScopedPtr<ResultCursor> QueryPlan::execute(size_t stmt_idx) {
  if (!scheduler_) {
    RAISE(kRuntimeError, "QueryPlan has no scheduler");
  }

  if (stmt_idx >= qtrees_.size()) {
    RAISE(kIndexError, "invalid statement index");
  }

  auto sched = scheduler_(txn_, &tasks_, &callbacks_);
  Set<TaskID> task_ids;
  for (const auto& task_id : statement_tasks_[stmt_idx]) {
    task_ids.emplace(task_id);
  }
  return sched->execute(task_ids);
}

void QueryPlan::execute(size_t stmt_idx, ResultList* result_list) {
  if (stmt_idx >= qtrees_.size()) {
    RAISE(kIndexError, "invalid statement index");
  }

  auto result_columns = getStatementOutputColumns(stmt_idx);
  auto result_cursor = execute(stmt_idx);

  result_list->addHeader(result_columns);
  Vector<SValue> tmp(result_columns.size());
  while (result_cursor->next(tmp.data(), tmp.size())) {
    result_list->addRow(tmp.data(), tmp.size());
  }
}

void QueryPlan::setScheduler(SchedulerFactory scheduler) {
  scheduler_ = scheduler;
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

//void QueryPlan::storeResults(size_t stmt_idx, ResultList* result_list) {
//
//  onOutputRow(
//      stmt_idx,
//      std::bind(
//          &ResultList::addRow,
//          result_list,
//          std::placeholders::_1,
//          std::placeholders::_2));
//}

}
