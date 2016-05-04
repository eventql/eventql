/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/tasks/subquery.h>
#include <csql/runtime/vm.h>

namespace csql {

Subquery::Subquery(
    Transaction* txn,
    Vector<ValueExpression> select_expressions,
    Option<ValueExpression> where_expr,
    HashMap<TaskID, ScopedPtr<ResultCursor>> input) :
    txn_(txn),
    select_exprs_(std::move(select_expressions)),
    where_expr_(std::move(where_expr)),
    input_(new ResultCursorList(std::move(input))) {}

bool Subquery::nextRow(SValue* out, int out_len) {
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

SubqueryFactory::SubqueryFactory(
    Vector<RefPtr<SelectListNode>> select_exprs,
    Option<RefPtr<ValueExpressionNode>> where_expr) :
    select_exprs_(select_exprs),
    where_expr_(where_expr) {}

RefPtr<Task> SubqueryFactory::build(
    Transaction* txn,
    HashMap<TaskID, ScopedPtr<ResultCursor>> input) const {
  Vector<ValueExpression> select_expressions;
  Option<ValueExpression> where_expr;

  auto qbuilder = txn->getRuntime()->queryBuilder();

  if (!where_expr_.isEmpty()) {
    where_expr = std::move(Option<ValueExpression>(
        qbuilder->buildValueExpression(txn, where_expr_.get())));
  }

  for (const auto& slnode : select_exprs_) {
    select_expressions.emplace_back(
        qbuilder->buildValueExpression(txn, slnode->expression()));
  }

  return new Subquery(
      txn,
      std::move(select_expressions),
      std::move(where_expr),
      std::move(input));
}

}
