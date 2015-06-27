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
    RefPtr<GroupByNode> node,
    DefaultRuntime* runtime) :
    source_(runtime->buildTableExpression(node->inputTable())) {
  for (const auto& slnode : node->selectList()) {
    select_exprs_.emplace_back(
        runtime->buildScalarExpression(slnode->expression()));
  }

  for (const auto& e : node->groupExpressions()) {
    group_exprs_.emplace_back(runtime->buildScalarExpression(e));
  }
}

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
    //auto& row = pair.second.row;
    for (size_t i = 0; i < select_exprs_.size(); ++i) {
      select_exprs_[i]->evaluate(
          &group.second[i],
          0,
          nullptr,
          &out_row[i]);
    }

    if (!fn(out_row.size(), out_row.data())) {
      return;
    }
  }
}

bool GroupBy::nextRow(int row_len, const SValue* row) {
  Vector<SValue> groups(group_exprs_.size(), SValue{});
  for (size_t i = 0; i < group_exprs_.size(); ++i) {
    group_exprs_[i]->evaluateStatic(row_len, row, &groups[i]);
  }

  auto group_key = SValue::makeUniqueKey(groups.data(), groups.size());
  auto& group = groups_[group_key];
  if (group.size() == 0) {
    for (const auto& e : select_exprs_) {
      group.emplace_back(e->allocInstance(&scratch_));
    }
  }

  //fnord::iputs("got row: $0 -> $1 -> $2", row_len, groups, group_key);

  for (size_t i = 0; i < select_exprs_.size(); ++i) {
    select_exprs_[i]->accumulate(&group[i], row_len, row);
  }

  //std::vector<SValue> row_vec;
  //for (int i = 0; i < out_len; i++) {
  //  row_vec.push_back(out[i]);
  //}

  ///* update group */
  //group->row = row_vec;

  return true;
}

  //size_t getNumCols() const override {
  //  return columns_.size();
  //}

  //const std::vector<std::string>& getColumns() const override {
  //  return columns_;
  //}

}
