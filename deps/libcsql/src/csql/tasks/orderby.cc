/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2011-2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <algorithm>
#include <csql/tasks/orderby.h>
#include <csql/expressions/boolean.h>
#include <csql/runtime/runtime.h>
#include <stx/inspect.h>

namespace csql {

OrderBy::OrderBy(
    Transaction* ctx,
    Vector<SortExpr> sort_specs,
    size_t num_columns,
    HashMap<TaskID, ScopedPtr<ResultCursor>> input) :
    ctx_(ctx),
    sort_specs_(std::move(sort_specs)),
    num_columns_(num_columns),
    input_(new ResultCursorList(std::move(input))) {
  if (sort_specs_.size() == 0) {
    RAISE(kIllegalArgumentError, "can't execute ORDER BY: no sort specs");
  }
}

bool OrderBy::nextRow(SValue* out, int out_len) {
  return false;
}
// FIXPAUL this should mergesort while inserting...
//bool OrderBy::onInputRow(
//    const TaskID& input_id,
//    const SValue* argv,
//    int argc) {
//  Vector<SValue> row;
//  for (int i = 0; i < argc; i++) {
//    row.emplace_back(argv[i]);
//  }
//
//  rows_.emplace_back(row);
//  return true;
//}
//
//void OrderBy::onInputsReady() {
//  auto rt = ctx_->getRuntime();
//  std::sort(
//      rows_.begin(),
//      rows_.end(),
//      [this, rt] (const Vector<SValue>& left, const Vector<SValue>& right) -> bool {
//    for (const auto& sort : sort_specs_) {
//      SValue args[2];
//      SValue res(false);
//      args[0] = rt->evaluateScalarExpression(
//          ctx_,
//          sort.expr,
//          left.size(),
//          left.data());
//      args[1] = rt->evaluateScalarExpression(
//          ctx_,
//          sort.expr,
//          right.size(),
//          right.data());
//
//      expressions::eqExpr(Transaction::get(ctx_), 2, args, &res);
//      if (res.getBool()) {
//        continue;
//      }
//
//      if (sort.descending) {
//        expressions::gtExpr(Transaction::get(ctx_), 2, args, &res);
//      } else {
//        expressions::ltExpr(Transaction::get(ctx_), 2, args, &res);
//      }
//
//      return res.getBool();
//    }
//
//    /* all dimensions equal */
//    return false;
//  });
//
//  for (auto& row : rows_) {
//    if (!input_(row.data(), row.size())) {
//      break;
//    }
//  }
//
//  rows_.clear();
//}

OrderByFactory::OrderByFactory(
    Vector<SortExpr> sort_specs,
    size_t num_columns) :
    sort_specs_(sort_specs),
    num_columns_(num_columns) {}

RefPtr<Task> OrderByFactory::build(
    Transaction* txn,
    HashMap<TaskID, ScopedPtr<ResultCursor>> input) const {
  auto qbuilder = txn->getRuntime()->queryBuilder();

  Vector<OrderBy::SortExpr> sort_exprs;
  for (const auto& ss : sort_specs_) {
    OrderBy::SortExpr se;
    se.descending = ss.descending;
    se.expr = qbuilder->buildValueExpression(txn, ss.expr);
    sort_exprs.emplace_back(std::move(se));
  }

  return new OrderBy(txn, std::move(sort_exprs), num_columns_, std::move(input));
}

} // namespace csql
