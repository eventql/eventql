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
    Vector<ValueExpression> select_expressions,
    Option<ValueExpression> where_expr,
    ScopedPtr<TableExpression> input) :
    txn_(txn),
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

//bool Subquery::onInputRow(
//    const TaskID& input_id,
//    const SValue* row,
//    int row_len) {
//  if (!where_expr_.isEmpty()) {
//    SValue pred;
//    VM::evaluate(txn_, where_expr_.get().program(), row_len, row, &pred);
//    if (!pred.getBool()) {
//      return true;
//    }
//  }
//
//  Vector<SValue> out_row(select_exprs_.size(), SValue{});
//  for (int i = 0; i < select_exprs_.size(); ++i) {
//    VM::evaluate(txn_, select_exprs_[i].program(), row_len, row, &out_row[i]);
//  }
//
//  return input_(out_row.data(), out_row.size());
//}

//SubqueryFactory::SubqueryFactory(
//    Vector<RefPtr<SelectListNode>> select_exprs,
//    Option<RefPtr<ValueExpressionNode>> where_expr) :
//    select_exprs_(select_exprs),
//    where_expr_(where_expr) {}
//
//RefPtr<Task> SubqueryFactory::build(
//    Transaction* txn,
//    HashMap<TaskID, ScopedPtr<ResultCursor>> input) const {
//  Vector<ValueExpression> select_expressions;
//  Option<ValueExpression> where_expr;
//
//  auto qbuilder = txn->getRuntime()->queryBuilder();
//
//  if (!where_expr_.isEmpty()) {
//    where_expr = std::move(Option<ValueExpression>(
//        qbuilder->buildValueExpression(txn, where_expr_.get())));
//  }
//
//  for (const auto& slnode : select_exprs_) {
//    select_expressions.emplace_back(
//        qbuilder->buildValueExpression(txn, slnode->expression()));
//  }
//
//  return new Subquery(
//      txn,
//      std::move(select_expressions),
//      std::move(where_expr),
//      std::move(input));
//}

}
