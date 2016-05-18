/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *   - Laura Schlimmer <laura@zscale.io>
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
#include <eventql/sql/expressions/table/select.h>

namespace csql {

SelectExpression::SelectExpression(
    Transaction* txn,
    ExecutionContext* execution_context,
    Vector<ValueExpression> select_expressions) :
    txn_(txn),
    execution_context_(execution_context),
    select_exprs_(std::move(select_expressions)),
    pos_(0) {
  execution_context_->incrementNumTasks();
}

ScopedPtr<ResultCursor> SelectExpression::execute() {
  execution_context_->incrementNumTasksRunning();

  return mkScoped(
      new DefaultResultCursor(
          select_exprs_.size(),
          std::bind(
              &SelectExpression::next,
              this,
              std::placeholders::_1,
              std::placeholders::_2)));
}

size_t SelectExpression::getNumColumns() const {
  return select_exprs_.size();
}

bool SelectExpression::next(SValue* row, int row_len) {
  if (pos_++ == 0) {
    for (int i = 0; i < select_exprs_.size() && i < row_len; ++i) {
      VM::evaluate(txn_, select_exprs_[i].program(), 0, nullptr,  &row[i]);
    }

    execution_context_->incrementNumTasksCompleted();

    return true;
  } else {
    return false;
  }
}

//bool Select::nextRow(SValue* out, int out_len) {
//  if (pos_ == 0) {
//    for (int i = 0; i < select_exprs_.size() && i < out_len; ++i) {
//      VM::evaluate(txn_, select_exprs_[i].program(), 0, nullptr,  &out[i]);
//    }
//
//    ++pos_;
//    return true;
//  } else {
//    return false;
//  }
//}

//SelectFactory::SelectFactory(
//    Vector<RefPtr<SelectListNode>> select_exprs) :
//    select_exprs_(select_exprs) {}
//
//RefPtr<Task> SelectFactory::build(
//    Transaction* txn,
//    HashMap<TaskID, ScopedPtr<ResultCursor>> input) const {
//  Vector<ValueExpression> select_expressions;
//  auto qbuilder = txn->getRuntime()->queryBuilder();
//  for (const auto& slnode : select_exprs_) {
//    select_expressions.emplace_back(
//        qbuilder->buildValueExpression(txn, slnode->expression()));
//  }
//
//  return new Select(txn, std::move(select_expressions));
//}
//
}
