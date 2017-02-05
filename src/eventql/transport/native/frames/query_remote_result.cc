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
#include <assert.h>
#include "eventql/transport/native/frames/query_remote_result.h"
#include "eventql/util/util/binarymessagereader.h"

namespace eventql {
namespace native_transport {

QueryRemoteResultFrame::QueryRemoteResultFrame() :
    flags_(0),
    row_count_(0) {}

size_t QueryRemoteResultFrame::getColumnCount() const {
  return column_data_.size();
}

const std::string& QueryRemoteResultFrame::getColumnData(size_t idx) const {
  assert(idx < column_data_.size());
  return column_data_[idx];
}

void QueryRemoteResultFrame::addColumnData(const char* data, size_t size) {
  column_data_.emplace_back(data, size);
}

size_t QueryRemoteResultFrame::getRowCount() const {
  return row_count_;
}

void QueryRemoteResultFrame::setRowCount(size_t row_count) {
  row_count_ = row_count;
}

ReturnCode QueryRemoteResultFrame::parseFrom(const char* data, size_t size) {
  MemoryInputStream is(data, size);
  return parseFrom(&is);
}

ReturnCode QueryRemoteResultFrame::parseFrom(InputStream* is) {
  flags_ = is->readVarUInt();
  row_count_ = is->readVarUInt();

  auto column_count = is->readVarUInt();
  for (size_t i = 0; i < column_count; ++i) {
    column_data_.emplace_back(is->readLenencString());
  }

  return ReturnCode::success();
}

void QueryRemoteResultFrame::writeToString(std::string* string) const {
  auto os = StringOutputStream::fromString(string);
  writeTo(os.get());
}

void QueryRemoteResultFrame::writeTo(OutputStream* os) const {
  os->appendVarUInt(flags_);
  os->appendVarUInt(row_count_);
  os->appendVarUInt(column_data_.size());
  for (const auto& d : column_data_) {
    os->appendLenencString(d);
  }
}

void QueryRemoteResultFrame::clear() {
  flags_ = 0;
  row_count_ = 0;
  column_data_.clear();
}

} // namespace native_transport
} // namespace eventql

