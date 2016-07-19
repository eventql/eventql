/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
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
#pragma once
#include "eventql/eventql.h"
#include <eventql/util/stdtypes.h>
#include <eventql/util/buffer.h>
#include <eventql/util/SHA1.h>
#include <eventql/util/io/inputstream.h>
#include <eventql/util/io/outputstream.h>
#include <eventql/util/protobuf/msg.h>
#include <eventql/util/protobuf/DynamicMessage.h>

namespace eventql {

struct ShreddedRecordValue {
  uint32_t rlvl;
  uint32_t dlvl;
  String value;
};

struct ShreddedRecordColumn {
  String column_name;
  Vector<ShreddedRecordValue> values;
  void addValue(uint32_t rlvl, uint32_t dlvl, String value);
  void addNull(uint32_t rlvl, uint32_t dlvl);
};

class ShreddedRecordList {
public:

  ShreddedRecordList();
  ShreddedRecordList(
      Vector<SHA1Hash> record_ids,
      Vector<uint64_t> record_versions,
      List<ShreddedRecordColumn> columns);

  size_t getNumColumns() const;
  size_t getNumRecords() const;

  const SHA1Hash& getRecordID(size_t idx) const;
  uint64_t getRecordVersion(size_t idx) const;
  const ShreddedRecordColumn* getColumn(size_t idx) const;

  void encode(OutputStream* os) const;
  void decode(InputStream* is);

  void debugPrint() const;

protected:
  Vector<SHA1Hash> record_ids_;
  Vector<uint64_t> record_versions_;
  Vector<ShreddedRecordColumn> columns_;
};

class ShreddedRecordListBuilder {
public:

  ShreddedRecordListBuilder();

  ShreddedRecordColumn* getColumn(const String& column_name);

  void addRecord(const SHA1Hash& record_id, uint64_t record_version);

  void addRecordFromProtobuf(
      const SHA1Hash& record_id,
      uint64_t record_version,
      const msg::DynamicMessage& msg);

  void addRecordFromProtobuf(
      const SHA1Hash& record_id,
      uint64_t record_version,
      const msg::MessageObject& msg,
      const msg::MessageSchema& schema);

  ShreddedRecordList get();

protected:
  Vector<SHA1Hash> record_ids_;
  Vector<uint64_t> record_versions_;
  List<ShreddedRecordColumn> columns_;
  HashMap<String, ShreddedRecordColumn*> columns_by_name_;
};

} // namespace eventql

