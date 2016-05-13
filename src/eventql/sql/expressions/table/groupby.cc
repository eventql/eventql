/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <eventql/util/io/BufferedOutputStream.h>
#include <eventql/util/io/fileutil.h>
#include <eventql/sql/expressions/table/groupby.h>

namespace csql {

GroupByExpression::GroupByExpression(
    Transaction* txn,
    Vector<ValueExpression> select_expressions,
    Vector<ValueExpression> group_expressions,
    ScopedPtr<TableExpression> input) :
    txn_(txn),
    select_exprs_(std::move(select_expressions)),
    group_exprs_(std::move(group_expressions)),
    input_(std::move(input)),
    freed_(false) {}

GroupByExpression::~GroupByExpression() {
  if (!freed_) {
    freeResult();
  }
}

ScopedPtr<ResultCursor> GroupByExpression::execute() {
  auto input_cursor = input_->execute();
  Vector<SValue> row(input_cursor->getNumColumns());
  while (input_cursor->next(row.data(), row.size())) {
    Vector<SValue> gkey(group_exprs_.size(), SValue{});
    for (size_t i = 0; i < group_exprs_.size(); ++i) {
      VM::evaluate(
          txn_,
          group_exprs_[i].program(),
          row.size(),
          row.data(),
          &gkey[i]);
    }

    auto group_key = SValue::makeUniqueKey(gkey.data(), gkey.size());
    auto& group = groups_[group_key];
    if (group.size() == 0) {
      for (const auto& e : select_exprs_) {
        group.emplace_back(VM::allocInstance(txn_, e.program(), &scratch_));
      }
    }

    for (size_t i = 0; i < select_exprs_.size(); ++i) {
      VM::accumulate(
          txn_,
          select_exprs_[i].program(),
          &group[i],
          row.size(),
          row.data());
    }
  }

  groups_iter_ = groups_.begin();
  return mkScoped(
      new DefaultResultCursor(
          select_exprs_.size(),
          std::bind(
              &GroupByExpression::next,
              this,
              std::placeholders::_1,
              std::placeholders::_2)));
}

bool GroupByExpression::next(SValue* row, size_t row_len) {
  if (groups_iter_ != groups_.end()) {
    for (size_t i = 0; i < select_exprs_.size(); ++i) {
      VM::result(txn_, select_exprs_[i].program(), &groups_iter_->second[i], &row[i]);
    }

    if (++groups_iter_ == groups_.end()) {
      freeResult();
    }
    return true;
  } else {
    return false;
  }
}

void GroupByExpression::freeResult() {
  for (auto& group : groups_) {
    for (size_t i = 0; i < select_exprs_.size(); ++i) {
      VM::freeInstance(txn_, select_exprs_[i].program(), &group.second[i]);
    }
  }

  groups_.clear();
  freed_ = true;
}

} // namespace csql
