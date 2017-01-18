/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#include "eventql/transport/native/frames/insert.h"

namespace eventql {
namespace native_transport {

InsertFrame::InsertFrame() : flags_(0), record_encoding_(0) {}

void InsertFrame::setDatabase(const std::string& database) {
  database_ = database;
}

void InsertFrame::setTable(const std::string& table) {
  table_ = table;
}

void InsertFrame::setRecordEncoding(uint64_t encoding) {
  record_encoding_ = encoding;
}

void InsertFrame::setRecordEncodingInfo(const std::string& str) {
  flags_ |= EVQL_INSERT_HAS_ENCODING_INFO;
  record_encoding_info_ = str;
}

void InsertFrame::addRecord(const std::string& record) {
  records_.emplace_back(record);
}

const std::string& InsertFrame::getDatabase() const {
  return database_;
}

const std::string& InsertFrame::getTable() const {
  return table_;
}

uint64_t InsertFrame::getRecordEncoding() const {
  return record_encoding_;
}

const std::string& InsertFrame::getRecordEncodingInfo() const {
  return record_encoding_info_;
}

const std::vector<std::string>& InsertFrame::getRecords() const {
  return records_;
}

ReturnCode InsertFrame::parseFrom(InputStream* is) try {
  flags_ = is->readVarUInt();
  database_ = is->readLenencString();
  table_ = is->readLenencString();
  record_encoding_ = is->readVarUInt();
  if (flags_ & EVQL_INSERT_HAS_ENCODING_INFO) {
    record_encoding_info_ = is->readLenencString();
  }

  auto records_count = is->readVarUInt();
  for (uint64_t i = 0; i < records_count; ++i) {
    records_.emplace_back(is->readLenencString());
  }

  return ReturnCode::success();
} catch (const std::exception& e) {
  return ReturnCode::exception(e);
}

ReturnCode InsertFrame::writeTo(OutputStream* os) const try {
  os->appendVarUInt(flags_);
  os->appendLenencString(database_);
  os->appendLenencString(table_);
  os->appendVarUInt(record_encoding_);
  if (flags_ & EVQL_INSERT_HAS_ENCODING_INFO) {
    os->appendLenencString(record_encoding_info_);
  }

  os->appendVarUInt(records_.size());
  for (const auto& r : records_) {
    os->appendLenencString(r);
  }

  return ReturnCode::success();
} catch (const std::exception& e) {
  return ReturnCode::exception(e);
}

void InsertFrame::clear() {
  flags_ = 0;
  database_.clear();
  table_.clear();
  record_encoding_ = 0;
  records_.clear();
}

} // namespace native_transport
} // namespace eventql

