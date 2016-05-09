/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <eventql/sql/runtime/queryplan.h>
#include <eventql/sql/runtime/runtime.h>

namespace csql {

QueryPlan::QueryPlan(
    Transaction* txn,
    Vector<RefPtr<QueryTreeNode>> qtrees) :
    txn_(txn),
    qtrees_(qtrees),
    scheduler_(nullptr) {
  //for (const auto& qtree : qtrees_) {
  //  statement_tasks_.emplace_back(
  //      qtree.asInstanceOf<TableExpressionNode>()->build(txn, &tasks_));

  //  statement_columns_.emplace_back(
  //      qtree.asInstanceOf<TableExpressionNode>()->outputColumns());
  //}
}

ScopedPtr<ResultCursor> QueryPlan::execute(size_t stmt_idx) {
  if (scheduler_.isNull()) {
    RAISE(kRuntimeError, "QueryPlan has no scheduler");
  }

  if (stmt_idx >= qtrees_.size()) {
    RAISE(kIndexError, "invalid statement index");
  }

  return scheduler_->execute(this, stmt_idx);
}

void QueryPlan::execute(size_t stmt_idx, ResultList* result_list) {
  if (stmt_idx >= qtrees_.size()) {
    RAISE(kIndexError, "invalid statement index");
  }

 // result_list->addHeader(getStatementOutputColumns(stmt_idx));

  auto cursor = execute(stmt_idx);
  Vector<SValue> tmp(cursor->getNumColumns());
  while (cursor->next(tmp.data(), tmp.size())) {
    result_list->addRow(tmp.data(), tmp.size());
  }
}

void QueryPlan::setScheduler(RefPtr<Scheduler> scheduler) {
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

RefPtr<QueryTreeNode> QueryPlan::getStatement(size_t stmt_idx) const {
  if (stmt_idx >= qtrees_.size()) {
    RAISE(kIndexError, "invalid statement index");
  }

  return qtrees_[stmt_idx];
}

Transaction* QueryPlan::getTransaction() const {
  return txn_;
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
