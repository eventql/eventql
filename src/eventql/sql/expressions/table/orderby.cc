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

OrderByExpression::OrderByExpression(
    Transaction* txn,
    Vector<SortExpr> sort_specs,
    ScopedPtr<TableExpression> input) :
    txn_(txn),
    sort_specs_(std::move(sort_specs)),
    input_(std::move(input)),
    num_rows_(0),
    pos_(0) {
  if (sort_specs_.size() == 0) {
    RAISE(kIllegalArgumentError, "can't execute ORDER BY: no sort specs");
  }
}

ScopedPtr<ResultCursor> OrderByExpression::execute() {
  input_cursor_ = input_->execute();

  Vector<SValue> row(input_cursor_->getNumColumns());
  while (input_cursor_->next(row.data(), row.size())) {
    rows_.emplace_back(row);
  }

  num_rows_ = rows_.size();

  std::sort(
      rows_.begin(),
      rows_.end(),
      [this] (const Vector<SValue>& left, const Vector<SValue>& right) -> bool {
    for (const auto& sort : sort_specs_) {
      SValue args[2];

      VM::evaluate(
          txn_,
          sort.expr.program(),
          left.size(),
          left.data(),
          &args[0]);

      VM::evaluate(
          txn_,
          sort.expr.program(),
          right.size(),
          right.data(),
          &args[1]);

      SValue res(false);
      expressions::eqExpr(Transaction::get(txn_), 2, args, &res);
      if (res.getBool()) {
        continue;
      }

      if (sort.descending) {
        expressions::gtExpr(Transaction::get(txn_), 2, args, &res);
      } else {
        expressions::ltExpr(Transaction::get(txn_), 2, args, &res);
      }

      return res.getBool();
    }

    /* all dimensions equal */
    return false;
  });

  return mkScoped(
      new DefaultResultCursor(
          input_cursor_->getNumColumns(),
          std::bind(
              &OrderByExpression::next,
              this,
              std::placeholders::_1,
              std::placeholders::_2)));
}

bool OrderByExpression::next(SValue* out, int out_len) {
  if (pos_ >= num_rows_) {
    return false;
  } else {
    for (size_t i = 0; i < input_cursor_->getNumColumns() && i < out_len; ++i) {
      out[i] = rows_[pos_][i];
    }

    if (++pos_ >= num_rows_) {
      rows_.clear();
    }

    return true;
  }
}

} // namespace csql
