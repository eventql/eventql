/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <eventql/sql/expressions/table/limit.h>

namespace csql {

Limit::Limit(
    size_t limit,
    size_t offset,
    ScopedPtr<TableExpression> input) :
    limit_(limit),
    offset_(offset),
    input_(std::move(input)),
    counter_(0) {}

ScopedPtr<ResultCursor> Limit::execute() {
  input_cursor_ = input_->execute();

  return mkScoped(
      new DefaultResultCursor(
          input_cursor_->getNumColumns(),
          std::bind(
              &Limit::next,
              this,
              std::placeholders::_1,
              std::placeholders::_2)));
}


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
bool Limit::next(SValue* row, size_t row_len) {
  if (counter_++ < offset_) {
    return true;
  }

  if (counter_ > (offset_ + limit_)) {
    return false;
  }

  return false;
}

}
