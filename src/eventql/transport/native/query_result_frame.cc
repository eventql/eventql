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
#include "eventql/transport/native/query_result_frame.h"

namespace eventql {
namespace native_transport {

QueryResultFrame::QueryResultFrame(
    const std::vector<std::string>& columns) :
    columns_(columns),
    is_last_(false),
    has_pending_stmt_(false),
    num_rows_(0) {}

void QueryResultFrame::addRow(const std::vector<csql::SValue>& row) {
  for (const auto& v : row) {
    data_.appendLenencString(v.getString());
  }

  ++num_rows_;
}

size_t QueryResultFrame::getRowCount() const {
  return num_rows_;
}

size_t QueryResultFrame::getRowBytes() const {
  return data_.size();
}

void QueryResultFrame::setIsLast(bool is_last) {
  is_last_ = is_last;
}

void QueryResultFrame::setHasPendingStatement(bool has_pending_stmt) {
  has_pending_stmt_ = has_pending_stmt;
}

void QueryResultFrame::writeTo(NativeConnection* conn) {
  uint64_t flags = 0;
  flags |= EVQL_QUERY_RESULT_HASCOLNAMES;
  flags |= EVQL_QUERY_RESULT_HASSTATS;
  if (is_last_) {
    flags |= EVQL_QUERY_RESULT_COMPLETE;
  }
  if (has_pending_stmt_) {
    flags |= EVQL_QUERY_RESULT_PENDINGSTMT;
  }

  util::BinaryMessageWriter header_;
  header_.appendVarUInt(flags); // flags
  header_.appendVarUInt(columns_.size());
  header_.appendVarUInt(num_rows_);

  //if (is_last_) {
    header_.appendVarUInt(0); // num_rows_modified
    header_.appendVarUInt(0); // num_rows_scanned
    header_.appendVarUInt(0); // num_bytes_scanned
    header_.appendVarUInt(0); // query_runtime_ms
  //}

  for (const auto& c : columns_) {
    header_.appendLenencString(c);
  }

  conn->writeFrameHeaderAsync(
      EVQL_OP_QUERY_RESULT,
      header_.size() + data_.size());

  conn->writeAsync(header_.data(), header_.size());
  conn->writeAsync(data_.data(), data_.size());
  conn->flushBuffer(true);
}

void QueryResultFrame::clear() {
  is_last_ = false;
  has_pending_stmt_ = false;
  num_rows_ = 0;
  data_.clear();
}


} // namespace native_transport
} // namespace eventql

