/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *   - Laura Schlimmer <laura@zscale.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#include <eventql/sql/expressions/table/limit.h>

namespace csql {

LimitExpression::LimitExpression(
    ExecutionContext* execution_context,
    size_t limit,
    size_t offset,
    ScopedPtr<TableExpression> input) :
    execution_context_(execution_context),
    limit_(limit),
    offset_(offset),
    input_(std::move(input)),
    counter_(0) {
  execution_context->incrementNumTasksRunning();
}

ScopedPtr<ResultCursor> LimitExpression::execute() {
  input_cursor_ = input_->execute();
  buf_.resize(input_cursor_->getNumColumns());

  return mkScoped(
      new DefaultResultCursor(
          input_cursor_->getNumColumns(),
          std::bind(
              &LimitExpression::next,
              this,
              std::placeholders::_1,
              std::placeholders::_2)));
}

size_t LimitExpression::getNumColumns() const {
  return input_cursor_->getNumColumns();
}

bool LimitExpression::next(SValue* row, size_t row_len) {
  if (limit_ == 0 || counter_ >= offset_ + limit_) {
    if (completion_callback_) {
      completion_callback_();
      completion_callback_ = nullptr;
    }
    return false;
  }

  while (input_cursor_->next(buf_.data(), buf_.size())) {
    if (counter_++ < offset_) {
      continue;
    } else {
      for (size_t i = 0; i < row_len && i < buf_.size(); ++i) {
        row[i] = buf_[i];
      }
      return true;
    }
  }

  if (completion_callback_) {
    completion_callback_();
    completion_callback_ = nullptr;
  }
  return false;
}


}
