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
#include <eventql/sql/expressions/table/orderby.h>
#include <eventql/sql/expressions/boolean.h>
#include <eventql/sql/runtime/runtime.h>
#include <eventql/util/inspect.h>

namespace csql {

OrderBy::OrderBy(
    Transaction* ctx,
    Vector<SortExpr> sort_specs,
    ScopedPtr<TableExpression> input) :
    ctx_(ctx),
    sort_specs_(std::move(sort_specs)),
    input_(std::move(input)) {
  if (sort_specs_.size() == 0) {
    RAISE(kIllegalArgumentError, "can't execute ORDER BY: no sort specs");
  }
}

ScopedPtr<ResultCursor> OrderByExpression::execute() {
  input_cursor_ = input_->execute();

  return mkScoped(
      new DefaultResultCursor(
          select_exprs_.size(),
          std::bind(
              &ORderByExpression::next,
              this,
              std::placeholders::_1,
              std::placeholders::_2)));
}

bool OrderBy::next(SValue* out, int out_len) {
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

} // namespace csql
