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
#include "eventql/transport/native/frames/query_remote.h"
#include "eventql/util/util/binarymessagereader.h"

namespace eventql {
namespace native_transport {

QueryRemoteFrame::QueryRemoteFrame() : flags_(0) {};

void QueryRemoteFrame::setDatabase(const std::string& database) {
  database_ = database;
}

const std::string& QueryRemoteFrame::getDatabase() const {
  return database_;
}

const std::string& QueryRemoteFrame::getEncodedQTree() const {
  return encoded_qtree_;
}

void QueryRemoteFrame::setEncodedQtree(const std::string& encoded_qtree) {
  encoded_qtree_ = encoded_qtree;
}

ReturnCode QueryRemoteFrame::parseFrom(const char* data, size_t size) {
  MemoryInputStream is(data, size);
  return parseFrom(&is);
}

ReturnCode QueryRemoteFrame::parseFrom(InputStream* is) {
  flags_ = is->readVarUInt();
  database_ = is->readLenencString();
  encoded_qtree_ = is->readLenencString();
  return ReturnCode::success();
}

void QueryRemoteFrame::writeToString(std::string* string) {
  auto os = StringOutputStream::fromString(string);
  writeTo(os.get());
}

void QueryRemoteFrame::writeTo(OutputStream* os) {
  os->appendVarUInt(flags_);
  os->appendLenencString(database_);
  os->appendLenencString(encoded_qtree_);
}

} // namespace native_transport
} // namespace eventql

