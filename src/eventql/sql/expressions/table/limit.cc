/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *   - Laura Schlimmer <laura@eventql.io>
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
    counter_(0) {}

ReturnCode LimitExpression::execute() {
  auto rc = input_->execute();
  if (!rc.isSuccess()) {
    return rc;
  }

  buf_.resize(input_->getColumnCount());
  return ReturnCode::success();
}

ReturnCode LimitExpression::nextBatch(
    size_t limit,
    SVector* columns,
    size_t* nrecords) {
  return ReturnCode::error("ERUNTIME", "LimitExpression::nextBatch not yet implemented");
}

size_t LimitExpression::getColumnCount() const {
  return input_->getColumnCount();
}

SType LimitExpression::getColumnType(size_t idx) const {
  return input_->getColumnType(idx);
}

bool LimitExpression::next(SValue* row, size_t row_len) {
  if (limit_ == 0 || counter_ >= offset_ + limit_) {
    return false;
  }

  while (input_->next(buf_.data(), buf_.size())) {
    if (counter_++ < offset_) {
      continue;
    } else {
      for (size_t i = 0; i < row_len && i < buf_.size(); ++i) {
        row[i] = buf_[i];
      }
      return true;
    }
  }

  return false;
}


}
