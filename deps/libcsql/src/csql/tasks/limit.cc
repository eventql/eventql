/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/tasks/limit.h>

namespace csql {

Limit::Limit(
    size_t limit,
    size_t offset,
    HashMap<TaskID, ScopedPtr<ResultCursor>> input) :
    limit_(limit),
    offset_(offset),
    input_(new ResultCursorList(std::move(input))),
    counter_(0) {}

//bool Limit::onInputRow(
//    const TaskID& input_id,
//    const SValue* row,
//    int row_len) {
//  if (counter_++ < offset_) {
//    return true;
//  }
//
//  if (counter_ > (offset_ + limit_)) {
//    return false;
//  }
//
//  return input_(row, row_len);
//}
bool Limit::nextRow(SValue* out, int out_len) {
  return false;
}

LimitFactory::LimitFactory(
    size_t limit,
    size_t offset) :
    limit_(limit),
    offset_(offset) {}

RefPtr<Task> LimitFactory::build(
    Transaction* txn,
    HashMap<TaskID, ScopedPtr<ResultCursor>> input) const {
  return new Limit(limit_, offset_, std::move(input));
}

}
