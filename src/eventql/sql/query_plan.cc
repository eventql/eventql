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
#include <eventql/sql/query_plan.h>
#include <eventql/sql/scheduler.h>
#include <eventql/sql/runtime/runtime.h>

namespace csql {

QueryPlan::QueryPlan(
    Transaction* txn,
    Vector<RefPtr<QueryTreeNode>> qtrees) :
    txn_(txn),
    qtrees_(qtrees),
    execution_contexts_(qtrees_.size()) {
  for (const auto& qtree : qtrees_) {
    if (qtree.isInstanceOf<TableExpressionNode>()) {
      statement_columns_.emplace_back(
          qtree.asInstanceOf<TableExpressionNode>()->getResultColumns());
    } else {
      statement_columns_.emplace_back();
    }
  }
}

ScopedPtr<ResultCursor> QueryPlan::execute(size_t stmt_idx) {
  if (stmt_idx >= qtrees_.size()) {
    RAISE(kIndexError, "invalid statement index");
  }

  auto execution_context = &execution_contexts_[stmt_idx];
  if (progress_callback_) {
    progress_callback_();
    execution_context->setProgressCallback(progress_callback_);
  }

  return txn_->getRuntime()->getScheduler()->execute(
      this,
      execution_context,
      stmt_idx);
}

void QueryPlan::execute(size_t stmt_idx, ResultList* result_list) {
  if (stmt_idx >= qtrees_.size()) {
    RAISE(kIndexError, "invalid statement index");
  }

  result_list->addHeader(getStatementgetResultColumns(stmt_idx));

  auto cursor = execute(stmt_idx);
  Vector<SValue> tmp(cursor->getNumColumns());
  while (cursor->next(tmp.data(), tmp.size())) {
    result_list->addRow(tmp.data(), tmp.size());
  }
}

const Vector<String>& QueryPlan::getStatementgetResultColumns(size_t stmt_idx) {
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

double QueryPlan::getQueryProgress(size_t query_idx) const {
  if (query_idx >= qtrees_.size()) {
    RAISE(kIndexError, "invalid statement index");
  }

  return execution_contexts_[query_idx].getProgress();
}

double QueryPlan::getProgress() const {
  size_t num_queries = qtrees_.size();
  double progress = 0;
  for (size_t i = 0; i < num_queries; ++i) {
    progress += execution_contexts_[i].getProgress();
  }

  return progress / num_queries;
}

void QueryPlan::setProgressCallback(Function<void()> cb) {
  progress_callback_ = cb;
}

Transaction* QueryPlan::getTransaction() const {
  return txn_;
}

}
