/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/runtime/groupby.h>

namespace csql {

GroupBy::GroupBy(
    ScopedPtr<TableExpression> source,
    const Vector<String>& column_names,
    Vector<ScopedPtr<ValueExpression>> select_expressions,
    Vector<ScopedPtr<ValueExpression>> group_expressions) :
    source_(std::move(source)),
    column_names_(column_names),
    select_exprs_(std::move(select_expressions)),
    group_exprs_(std::move(group_expressions)) {}

GroupBy::~GroupBy() {
  for (auto& group : groups_) {
    for (size_t i = 0; i < select_exprs_.size(); ++i) {
      select_exprs_[i]->freeInstance(&group.second[i]);
    }
  }
}

void GroupBy::execute(
    ExecutionContext* context,
    Function<bool (int argc, const SValue* argv)> fn) {
  source_->execute(
      context,
      std::bind(
          &GroupBy::nextRow,
          this,
          std::placeholders::_1,
          std::placeholders::_2));

  Vector<SValue> out_row(select_exprs_.size(), SValue{});
  for (auto& group : groups_) {
    for (size_t i = 0; i < select_exprs_.size(); ++i) {
      select_exprs_[i]->result(&group.second[i], &out_row[i]);
    }

    if (!fn(out_row.size(), out_row.data())) {
      return;
    }
  }
}

bool GroupBy::nextRow(int row_len, const SValue* row) {
  Vector<SValue> groups(group_exprs_.size(), SValue{});
  for (size_t i = 0; i < group_exprs_.size(); ++i) {
    group_exprs_[i]->evaluate(row_len, row, &groups[i]);
  }

  auto group_key = SValue::makeUniqueKey(groups.data(), groups.size());
  auto& group = groups_[group_key];
  if (group.size() == 0) {
    for (const auto& e : select_exprs_) {
      group.emplace_back(e->allocInstance(&scratch_));
    }
  }

  for (size_t i = 0; i < select_exprs_.size(); ++i) {
    select_exprs_[i]->accumulate(&group[i], row_len, row);
  }

  return true;
}

Vector<String> GroupBy::columnNames() const {
  return column_names_;
}

size_t GroupBy::numColumns() const {
  return column_names_.size();
}

}
