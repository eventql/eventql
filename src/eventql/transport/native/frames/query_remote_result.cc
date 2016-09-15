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
#include "eventql/transport/native/frames/query_remote_result.h"
#include "eventql/util/util/binarymessagereader.h"

namespace eventql {
namespace native_transport {

QueryRemoteResultFrame::QueryRemoteResultFrame() : flags_(0) {};

size_t QueryRemoteResultFrame::getColumnCount() const {
  return column_count_;
}

void QueryRemoteResultFrame::setColumnCount(size_t column_count) {
  column_count_ = column_count;
}

size_t QueryRemoteResultFrame::getRowCount() const {
  return row_count_;
}

void QueryRemoteResultFrame::setRowCount(size_t row_count) {
  row_count_ = row_count;
}

std::unique_ptr<InputStream> QueryRemoteResultFrame::getRowDataInputStream() {
  return StringInputStream::fromString(row_data_);
}

std::unique_ptr<OutputStream> QueryRemoteResultFrame::getRowDataOutputStream() {
  return StringOutputStream::fromString(&row_data_);
}

ReturnCode QueryRemoteResultFrame::parseFrom(const char* data, size_t size) {
  MemoryInputStream is(data, size);
  return parseFrom(&is);
}

ReturnCode QueryRemoteResultFrame::parseFrom(InputStream* is) {
  flags_ = is->readVarUInt();
  column_count_ = is->readVarUInt();
  row_count_ = is->readVarUInt();
  row_data_ = is->readLenencString();
  return ReturnCode::success();
}

void QueryRemoteResultFrame::writeToString(std::string* string) {
  auto os = StringOutputStream::fromString(string);
  writeTo(os.get());
}

void QueryRemoteResultFrame::writeTo(OutputStream* os) {
  os->appendVarUInt(flags_);
  os->appendVarUInt(column_count_);
  os->appendVarUInt(row_count_);
  os->appendLenencString(row_data_);
}

} // namespace native_transport
} // namespace eventql

