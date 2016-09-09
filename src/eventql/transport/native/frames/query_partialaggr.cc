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
#include "eventql/transport/native/frames/query_partialaggr.h"
#include "eventql/util/util/binarymessagereader.h"

namespace eventql {
namespace native_transport {


QueryPartialAggrFrame::QueryPartialAggrFrame() : flags_(0) {};

ReturnCode QueryPartialAggrFrame::parseFrom(
    const char* payload,
    size_t payload_size) {
  util::BinaryMessageReader frame(payload, payload_size);
  flags_ = frame.readVarUInt();
  database_ = frame.readLenencString();
  encoded_qtree_ = frame.readLenencString();

  return ReturnCode::success();
}

void QueryPartialAggrFrame::setDatabase(const std::string& database) {
  database_ = database;
}

const std::string& QueryPartialAggrFrame::getDatabase() const {
  return database_;
}

const std::string& QueryPartialAggrFrame::getEncodedQTree() const {
  return encoded_qtree_;
}

void QueryPartialAggrFrame::setEncodedQtree(const std::string& encoded_qtree) {
  encoded_qtree_ = encoded_qtree;
}

void QueryPartialAggrFrame::writeToString(std::string* str) {
  util::BinaryMessageWriter writer;
  writer.appendVarUInt(flags_);
  writer.appendLenencString(database_);
  writer.appendLenencString(encoded_qtree_);

  *str = std::string((const char*) writer.data(), writer.size());
}

} // namespace native_transport
} // namespace eventql

