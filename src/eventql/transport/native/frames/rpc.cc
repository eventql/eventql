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
#include "eventql/transport/native/frames/rpc.h"

namespace eventql {
namespace native_transport {


RPCFrame::RPCFrame() : flags_(0) {};

void RPCFrame::setMethod(const std::string& method) {
  method_ = method;
}

void RPCFrame::setContentType(const std::string& content_type) {
  content_type_ = content_type;
  flags_ |= EVQL_RPC_HASCTYPE;
}

void RPCFrame::setDatabase(const std::string& database) {
  database_ = database;
  flags_ |= EVQL_RPC_SWITCHDB;
}

void RPCFrame::setBody(const std::string& body) {
  body_ = body;
}

void RPCFrame::writeToString(std::string* str, bool header /*= true*/) {
  util::BinaryMessageWriter writer;

  if (header) {
    writer.appendNUInt16(EVQL_OP_RPC);
    writer.appendNUInt16(0); // flags
    writer.appendNUInt32(0); // frame size
  }

  writer.appendVarUInt(flags_);
  writer.appendLenencString(method_);
  if (flags_ & EVQL_RPC_HASCTYPE) {
    writer.appendLenencString(content_type_);
  }
  if (flags_ & EVQL_RPC_SWITCHDB) {
    writer.appendLenencString(database_);
  }
  writer.appendLenencString(body_);

  if (header) {
    writer.updateNUInt32(4, writer.size() - 8); // update frame size
  }

  *str = std::string((const char*) writer.data(), writer.size());
}

} // namespace native_transport
} // namespace eventql

