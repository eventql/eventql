/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *   - Christian Parpart <trapni@dawanda.com>
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
#include <assert.h>
#include "eventql/eventql.h"
#include <eventql/sql/result_cursor.h>
#include <eventql/sql/table_expression.h>

namespace csql {

ResultCursor::ResultCursor() : eof_(true), buffer_len_(0) {}

ResultCursor::ResultCursor(
    ScopedPtr<TableExpression> table_expression) :
    table_expression_(std::move(table_expression)),
    eof_(false),
    buffer_len_(0) {
  for (size_t i = 0; i < table_expression_->getColumnCount(); ++i) {
    buffer_.emplace_back(table_expression_->getColumnType(i));
    buffer_cur_.emplace_back(nullptr);
  }

  auto rc = table_expression_->execute();
  if (!rc.isSuccess()) {
    RAISE(kRuntimeError, rc.getMessage());
  }

  rc = next();
  if (!rc.isSuccess()) {
    RAISE(kRuntimeError, rc.getMessage());
  }
}

bool ResultCursor::isValid() const {
  return !eof_;
}

ReturnCode ResultCursor::next() {
  if (eof_) {
    return ReturnCode::error("EIO", "end of result reached");
  }

  if (buffer_len_ > 1) {
    --buffer_len_;

    for (size_t i = 0; i < buffer_.size(); ++i) {
      buffer_[i].next(const_cast<void**>(&buffer_cur_[i]));
    }

    return ReturnCode::success();
  } else {
    return nextBatch();
  }
}

ReturnCode ResultCursor::nextBatch() {
  if (eof_) {
    return ReturnCode::error("EIO", "end of result reached");
  }

  for (auto& b : buffer_) {
    b.clear();
  }

  auto rc = table_expression_->nextBatch(buffer_.data(), &buffer_len_);
  if (!rc.isSuccess()) {
    return rc;
  }

  if (buffer_len_ == 0) {
    eof_ = true;
  }

  for (size_t i = 0; i < buffer_.size(); ++i) {
    buffer_cur_[i] = buffer_[i].getData();
  }

  return ReturnCode::success();
}

size_t ResultCursor::getColumnCount() const {
  if (!table_expression_) {
    return 0;
  }

  return table_expression_->getColumnCount();
}

SType ResultCursor::getColumnType(size_t idx) const {
  return table_expression_->getColumnType(idx);
}

std::string ResultCursor::getColumnString(size_t idx) const {
  return sql_tostring(buffer_[idx].getType(), buffer_cur_[idx]);
}

const void* ResultCursor::getColumnData(size_t idx) const {
  assert(idx < buffer_cur_.size());
  return buffer_cur_[idx];
}

const void* ResultCursor::getColumnBuffer(size_t idx) const {
  assert(idx < buffer_.size());
  return buffer_[idx].getData();
}

size_t ResultCursor::getColumnBufferSize(size_t idx) const {
  assert(idx < buffer_.size());
  return buffer_[idx].getSize();
}

size_t ResultCursor::getBufferCount() const {
  return buffer_len_;
}

} // namespace csql

