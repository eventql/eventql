/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2011-2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/runtime/orderby.h>
#include <csql/expressions/boolean.h>
#include <algorithm>
#include <stx/inspect.h>

namespace csql {

OrderBy::OrderBy(
    Vector<OrderByNode::SortSpec> sort_specs,
    size_t max_output_column_index,
    ScopedPtr<TableExpression> child) :
    sort_specs_(sort_specs),
    max_output_column_index_(max_output_column_index),
    child_(std::move(child)) {
  if (sort_specs_.size() == 0) {
    RAISE(kIllegalArgumentError, "can't execute ORDER BY: no sort specs");
  }

  size_t max_sort_idx = 0;
  for (const auto& sspec : sort_specs_) {
    if (sspec.column > max_sort_idx) {
      max_sort_idx = sspec.column;
    }
  }

  const auto& child_columns = child_->numColumns();
  if (child_columns <= max_sort_idx) {
    RAISE(
        kRuntimeError,
        "can't execute ORDER BY: not enough columns in virtual table");
  }
}

void OrderBy::prepare(ExecutionContext* context) {
  context->incrNumSubtasksTotal(1);
  child_->prepare(context);
}

// FIXPAUL this should mergesort while inserting...
void OrderBy::execute(
    ExecutionContext* context,
    Function<bool (int argc, const SValue* argv)> fn) {
  Vector<Vector<SValue>> rows;

  child_->execute(context, [&rows] (int argc, const SValue* argv) -> bool {
    Vector<SValue> row;
    for (int i = 0; i < argc; i++) {
      row.emplace_back(argv[i]);
    }

    rows.emplace_back(row);
    return true;
  });

  std::sort(
      rows.begin(),
      rows.end(),
      [this] (const Vector<SValue>& left, const Vector<SValue>& right) -> bool {
    for (const auto& sort : sort_specs_) {
      SValue args[2];
      SValue res(false);
      args[0] = left[sort.column];
      args[1] = right[sort.column];

      expressions::eqExpr(2, args, &res);
      if (res.getBool()) {
        continue;
      }

      if (sort.descending) {
        expressions::gtExpr(2, args, &res);
      } else {
        expressions::ltExpr(2, args, &res);
      }

      return res.getBool();
    }

    /* all dimensions equal */
    return false;
  });

  context->incrNumSubtasksCompleted(1);

  for (auto& row : rows) {
    if (!fn(std::min(row.size(), max_output_column_index_), row.data())) {
      return;
    }
  }
}

Vector<String> OrderBy::columnNames() const {
  auto col_names = child_->columnNames();

  if (col_names.size() > max_output_column_index_) {
    col_names.erase(
        col_names.begin() + max_output_column_index_,
        col_names.end());
  }

  return col_names;
}

size_t OrderBy::numColumns() const {
  return std::min(child_->numColumns(), max_output_column_index_);
}

} // namespace csql
