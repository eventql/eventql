/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/io/BufferedOutputStream.h>
#include <stx/io/fileutil.h>
#include <csql/tasks/groupby.h>

namespace csql {

GroupBy::GroupBy(
    Transaction* txn,
    Vector<ValueExpression> select_expressions,
    Vector<ValueExpression> group_expressions,
    HashMap<TaskID, ScopedPtr<ResultCursor>> input) :
    txn_(txn),
    select_exprs_(std::move(select_expressions)),
    group_exprs_(std::move(group_expressions)),
    input_(new ResultCursorList(std::move(input))) {}

bool GroupBy::nextRow(SValue* out, int out_len) {
  return false;
}
//bool GroupBy::onInputRow(
//      const TaskID& input_id,
//      const SValue* row,
//      int row_len) {
//  Vector<SValue> gkey(group_exprs_.size(), SValue{});
//  for (size_t i = 0; i < group_exprs_.size(); ++i) {
//    VM::evaluate(txn_, group_exprs_[i].program(), row_len, row, &gkey[i]);
//  }
//
//  auto group_key = SValue::makeUniqueKey(gkey.data(), gkey.size());
//  auto& group = groups_[group_key];
//  if (group.size() == 0) {
//    for (const auto& e : select_exprs_) {
//      group.emplace_back(VM::allocInstance(txn_, e.program(), &scratch_));
//    }
//  }
//
//  for (size_t i = 0; i < select_exprs_.size(); ++i) {
//    VM::accumulate(txn_, select_exprs_[i].program(), &group[i], row_len, row);
//  }
//
//  return true;
//}
//
//void GroupBy::onInputsReady() {
//  try {
//    Vector<SValue> out_row(select_exprs_.size(), SValue{});
//    for (auto& group : groups_) {
//      for (size_t i = 0; i < select_exprs_.size(); ++i) {
//        VM::result(txn_, select_exprs_[i].program(), &group.second[i], &out_row[i]);
//      }
//
//      if (!input_(out_row.data(), out_row.size())) {
//        break;
//      }
//    }
//  } catch (...) {
//    freeResult();
//    throw;
//  }
//
//  freeResult();
//}

void GroupBy::freeResult() {
  for (auto& group : groups_) {
    for (size_t i = 0; i < select_exprs_.size(); ++i) {
      VM::freeInstance(txn_, select_exprs_[i].program(), &group.second[i]);
    }
  }

  groups_.clear();
}

//Option<SHA1Hash> GroupBy::cacheKey() const {
//  auto source_key = source_->cacheKey();
//  if (source_key.isEmpty()) {
//    return None<SHA1Hash>();
//  }
//
//  return Some(
//      SHA1::compute(
//          StringUtil::format(
//              "$0~$1",
//              source_key.get().toString(),
//              qtree_fingerprint_.toString())));
//}

GroupByFactory::GroupByFactory(
    Vector<RefPtr<SelectListNode>> select_exprs,
    Vector<RefPtr<ValueExpressionNode>> group_exprs) :
    select_exprs_(select_exprs),
    group_exprs_(group_exprs) {}

RefPtr<Task> GroupByFactory::build(
    Transaction* txn,
    HashMap<TaskID, ScopedPtr<ResultCursor>> input) const {
  Vector<ValueExpression> select_expressions;
  Vector<ValueExpression> group_expressions;

  for (const auto& slnode : select_exprs_) {
    select_expressions.emplace_back(
        txn->getRuntime()->queryBuilder()->buildValueExpression(
            txn,
            slnode->expression()));
  }

  for (const auto& e : group_exprs_) {
    group_expressions.emplace_back(
        txn->getRuntime()->queryBuilder()->buildValueExpression(txn, e));
  }

  return new GroupBy(
      txn,
      std::move(select_expressions),
      std::move(group_expressions),
      std::move(input));
}

} // namespace csql
