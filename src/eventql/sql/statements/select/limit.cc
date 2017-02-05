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
#include <eventql/sql/statements/select/limit.h>

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

  for (size_t i = 0; i < input_->getColumnCount(); ++i) {
    input_buffer_.emplace_back(input_->getColumnType(i));
  }

  return ReturnCode::success();
}

ReturnCode LimitExpression::nextBatch(SVector* output, size_t* nrecords) {
  *nrecords = 0;

  auto limit_abs = limit_ + offset_;

  while (limit_ > 0 && counter_ < offset_ + limit_) {
    for (auto& c : input_buffer_) {
      c.clear();
    }

    size_t input_nrecords = 0;
    {
      auto rc = input_->nextBatch(input_buffer_.data(), &input_nrecords);
      if (!rc.isSuccess()) {
        RAISE(kRuntimeError, rc.getMessage());
      }
    }

    if (input_nrecords == 0) {
      *nrecords = 0;
      break;
    }

    // copy full batch
    if (counter_ >= offset_ && counter_ + input_nrecords <= limit_abs) {
      for (size_t i = 0; i < input_buffer_.size(); ++i) {
        output[i].copyFrom(&input_buffer_[i]);
      }

      counter_ += input_nrecords;
      *nrecords = input_nrecords;
      break;
    }

    // skip full batch
    if (counter_ + input_nrecords <= offset_) {
      counter_ += input_nrecords;
      continue;
    }

    // copy partial batch
    size_t cpy_off = counter_ < offset_ ? offset_ - counter_ : 0;
    size_t cpy_len =
        std::min(input_nrecords - cpy_off, limit_abs - counter_ - cpy_off);

    for (size_t i = 0; i < input_buffer_.size(); ++i) {
      const auto& input_col = input_buffer_[i];

      const void* begin = input_col.getData();
      if (cpy_off > 0) {
        for (size_t n = 0; n < cpy_off; ++n) {
          input_col.next(const_cast<void**>(&begin));
        }
      }

      const void* end = begin;
      if (cpy_len + cpy_off == input_nrecords) {
        end = (const char*) input_col.getData() + input_col.getSize();
      } else {
        for (size_t n = 0; n < cpy_len; ++n) {
          input_col.next(const_cast<void**>(&end));
        }
      }

      output[i].append(
          (const char*) begin,
          (const char*) end - (const char*) begin);
    }

    counter_ += cpy_len + cpy_off;
    *nrecords = cpy_len;
    break;
  }

  return ReturnCode::success();
}

size_t LimitExpression::getColumnCount() const {
  return input_->getColumnCount();
}

SType LimitExpression::getColumnType(size_t idx) const {
  return input_->getColumnType(idx);
}

} // namespace csql

