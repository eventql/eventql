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

void GroupBy::execute(
    ExecutionContext* context,
    Function<bool (int argc, const SValue* argv)> fn) {
  HashMap<String, Vector<ValueExpression::Instance>> groups;
  ScratchMemory scratch;

  try {
    accumulate(&groups, &scratch, context);
    getResult(&groups, fn);
  } catch (...) {
    freeResult(&groups);
    throw;
  }

  freeResult(&groups);
}

void GroupBy::accumulate(
    HashMap<String, Vector<ValueExpression::Instance>>* groups,
    ScratchMemory* scratch,
    ExecutionContext* context) {
  source_->execute(
      context,
      std::bind(
          &GroupBy::nextRow,
          this,
          groups,
          scratch,
          std::placeholders::_1,
          std::placeholders::_2));
}

void GroupBy::getResult(
    const HashMap<String, Vector<ValueExpression::Instance>>* groups,
    Function<bool (int argc, const SValue* argv)> fn) {
  Vector<SValue> out_row(select_exprs_.size(), SValue{});
  for (auto& group : *groups) {
    for (size_t i = 0; i < select_exprs_.size(); ++i) {
      select_exprs_[i]->result(&group.second[i], &out_row[i]);
    }

    if (!fn(out_row.size(), out_row.data())) {
      return;
    }
  }
}

void GroupBy::freeResult(
    HashMap<String, Vector<ValueExpression::Instance>>* groups) {
  for (auto& group : (*groups)) {
    for (size_t i = 0; i < select_exprs_.size(); ++i) {
      select_exprs_[i]->freeInstance(&group.second[i]);
    }
  }
}

void GroupBy::mergeResult(
    const HashMap<String, Vector<ValueExpression::Instance>>* src,
    HashMap<String, Vector<ValueExpression::Instance>>* dst,
    ScratchMemory* scratch) {
  for (const auto& src_group : *src) {
    auto& dst_group = (*dst)[src_group.first];
    if (dst_group.size() == 0) {
      for (const auto& e : select_exprs_) {
        dst_group.emplace_back(e->allocInstance(scratch));
      }
    }

    for (size_t i = 0; i < select_exprs_.size(); ++i) {
      select_exprs_[i]->merge(&dst_group[i], &src_group.second[i]);
    }
  }
}

bool GroupBy::nextRow(
    HashMap<String, Vector<ValueExpression::Instance>>* groups,
    ScratchMemory* scratch,
    int row_len, const SValue* row) {
  Vector<SValue> gkey(group_exprs_.size(), SValue{});
  for (size_t i = 0; i < group_exprs_.size(); ++i) {
    group_exprs_[i]->evaluate(row_len, row, &gkey[i]);
  }

  auto group_key = SValue::makeUniqueKey(gkey.data(), gkey.size());
  auto& group = (*groups)[group_key];
  if (group.size() == 0) {
    for (const auto& e : select_exprs_) {
      group.emplace_back(e->allocInstance(scratch));
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
