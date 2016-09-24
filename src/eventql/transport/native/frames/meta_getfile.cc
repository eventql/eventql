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
#include "eventql/transport/native/frames/meta_getfile.h"

namespace eventql {
namespace native_transport {

MetaGetfileFrame::MetaGetfileFrame() : flags_(0) {}

void MetaGetfileFrame::setDatabase(const std::string& database) {
  database_ = database;
}

void MetaGetfileFrame::setTable(const std::string& table) {
  table_ = table;
}

void MetaGetfileFrame::setTransactionID(const std::string& txid) {
  txid_ = txid;
}

void MetaGetfileFrame::setLatestTransactionFlag(bool flag) {
  if (flag) {
    flags_ |= EVQL_META_GETFILE_LATESTTXN;
  } else {
    flags_ &= ~EVQL_META_GETFILE_LATESTTXN;
  }
}

const std::string& MetaGetfileFrame::getDatabase() const {
  return database_;
}

const std::string& MetaGetfileFrame::getTable() const {
  return table_;
}

const std::string& MetaGetfileFrame::getTransactionID() const {
  return txid_;
}

bool MetaGetfileFrame::getLatestTransactionFlag() {
  return flags_ & EVQL_META_GETFILE_LATESTTXN;
}

ReturnCode MetaGetfileFrame::parseFrom(InputStream* is) try {
  flags_ = is->readVarUInt();
  database_ = is->readLenencString();
  table_ = is->readLenencString();
  txid_ = is->readLenencString();

  return ReturnCode::success();
} catch (const std::exception& e) {
  return ReturnCode::exception(e);
}

ReturnCode MetaGetfileFrame::writeTo(OutputStream* os) const try {
  os->appendVarUInt(flags_);
  os->appendLenencString(database_);
  os->appendLenencString(table_);
  os->appendLenencString(txid_);

  return ReturnCode::success();
} catch (const std::exception& e) {
  return ReturnCode::exception(e);
}

void MetaGetfileFrame::clear() {
  flags_ = 0;
  database_.clear();
  table_.clear();
  txid_.clear();
}

} // namespace native_transport
} // namespace eventql

