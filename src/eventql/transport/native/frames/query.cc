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
#include "eventql/transport/native/frames/query.h"

namespace eventql {
namespace native_transport {

QueryFrame::QueryFrame() : flags_(0), maxrows_(0) {};

void QueryFrame::setQuery(const std::string& query) {
  query_ = query;
}

const std::string& QueryFrame::getQuery() const {
  return query_;
}

void QueryFrame::setDatabase(const std::string& database) {
  database_ = database;
  flags_ |= EVQL_QUERY_SWITCHDB;
}

const std::string& QueryFrame::getDatabase() const {
  return database_;
}

ReturnCode QueryFrame::parseFrom(InputStream* is) {
  query_ = is->readLenencString();
  flags_ = is->readVarUInt();
  maxrows_ = is->readVarUInt();

  if (flags_ & EVQL_QUERY_SWITCHDB) {
    database_ = is->readLenencString();
  }

  return ReturnCode::success();
}

void QueryFrame::writeTo(OutputStream* os) const {
  os->appendLenencString(query_);
  os->appendVarUInt(flags_);
  os->appendVarUInt(maxrows_);
  if (flags_ & EVQL_QUERY_SWITCHDB) {
    os->appendLenencString(database_);
  }
}

} // namespace native_transport
} // namespace eventql

