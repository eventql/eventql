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
#include <eventql/sql/expressions/table/subquery.h>
#include <eventql/sql/runtime/vm.h>

namespace csql {

SubqueryExpression::SubqueryExpression(
    Transaction* txn,
    ExecutionContext* execution_context,
    Vector<ValueExpression> select_expressions,
    Option<ValueExpression> where_expr,
    ScopedPtr<TableExpression> input) :
    txn_(txn),
    execution_context_(execution_context),
    select_exprs_(std::move(select_expressions)),
    where_expr_(std::move(where_expr)),
    input_(std::move(input)) {}

ScopedPtr<ResultCursor> SubqueryExpression::execute() {
  input_cursor_ = input_->execute();
  buf_.resize(input_cursor_->getNumColumns());

  return mkScoped(
      new DefaultResultCursor(
          select_exprs_.size(),
          std::bind(
              &SubqueryExpression::next,
              this,
              std::placeholders::_1,
              std::placeholders::_2)));
}

size_t SubqueryExpression::getNumColumns() const {
  return select_exprs_.size();
}

bool SubqueryExpression::next(SValue* row, int row_len) {
  Vector<SValue> buf_(input_cursor_->getNumColumns());

  while (input_cursor_->next(buf_.data(), buf_.size())) {
    if (!where_expr_.isEmpty()) {
      SValue pred;
      VM::evaluate(
          txn_,
          where_expr_.get().program(),
          buf_.size(),
          buf_.data(),
          &pred);

      if (!pred.getBool()) {
        continue;
      }
    }

    for (size_t i = 0; i < select_exprs_.size() && i < row_len; ++i) {
      VM::evaluate(
          txn_,
          select_exprs_[i].program(),
          buf_.size(),
          buf_.data(),
          row + i);
    }

    return true;
  }

  return false;
}

}
