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
#include <eventql/db/shredded_record.h>

namespace eventql {

void ShreddedRecordColumn::addValue(
    uint32_t rlvl,
    uint32_t dlvl,
    String value) {
  ShreddedRecordValue v;
  v.rlvl = rlvl;
  v.dlvl = dlvl;
  v.value = value;
  values.emplace_back(v);
}

void ShreddedRecordColumn::addNull(
    uint32_t rlvl,
    uint32_t dlvl) {
  ShreddedRecordValue v;
  v.rlvl = rlvl;
  v.dlvl = dlvl;
  values.emplace_back(v);
}

ShreddedRecordList::ShreddedRecordList() {}

ShreddedRecordList::ShreddedRecordList(
    Vector<SHA1Hash> record_ids,
    Vector<uint64_t> record_versions,
    List<ShreddedRecordColumn> columns) :
    record_ids_(std::move(record_ids)),
    record_versions_(std::move(record_versions)),
    columns_(std::move(columns)) {}

size_t ShreddedRecordList::getNumColumns() const {
  return columns_.size();
}

size_t ShreddedRecordList::getNumRecords() const {
  return record_ids_.size();
}

//const SHA1Hash& getRecordID(size_t idx) const;
//uint64_t getRecordVersion(size_t idx) const;
//const ShreddedRecordColumn* getColumn(size_t idx) const;

void ShreddedRecordList::encode(OutputStream* os) const {
  os->appendUInt8(0x1);
  os->appendVarUInt(record_ids_.size());
  os->appendVarUInt(columns_.size());

  for (const auto& id : record_ids_) {
    os->write((const char*) id.data(), id.size());
  }
  for (auto version : record_versions_) {
    os->appendVarUInt(version);
  }

  for (const auto& c : columns_) {
    os->appendLenencString(c.column_name);
    os->appendVarUInt(c.values.size());
    for (const auto& v : c.values) {
      os->appendVarUInt(v.dlvl);
      os->appendVarUInt(v.rlvl);
      os->appendLenencString(v.value);
    }
  }
}

void ShreddedRecordList::decode(InputStream* is) {
  is->readUInt8();
  auto nrecs = is->readVarUInt();
  auto ncols = is->readVarUInt();

  record_ids_.reserve(nrecs);
  for (uint64_t i = 0; i < nrecs; ++i) {
    SHA1Hash id;
    is->readNextBytes(id.mutableData(), id.size());
    record_ids_.emplace_back(id);
  }

  record_versions_.reserve(nrecs);
  for (uint64_t i = 0; i < nrecs; ++i) {
    record_versions_.emplace_back(is->readVarUInt());
  }

  for (uint64_t j = 0; j < ncols; ++j) {
    columns_.emplace_back();
    auto& col = columns_.back();
    col.column_name = is->readLenencString();
    auto len = is->readVarUInt();
    col.values.reserve(len);
    for (uint64_t i = 0; i < len; ++i) {
      col.values.emplace_back();
      auto& val = col.values.back();
      val.dlvl = is->readVarUInt();
      val.rlvl = is->readVarUInt();
      val.value = is->readLenencString();
    }
  }
}

ShreddedRecordListBuilder::ShreddedRecordListBuilder() {}

ShreddedRecordColumn* ShreddedRecordListBuilder::addColumn(
    const String& column_name) {
  columns_.emplace_back();
  auto col = &columns_.back();
  col->column_name = column_name;
  return col;
}

void ShreddedRecordListBuilder::addRecord(
    const SHA1Hash& record_id,
    uint64_t record_version) {
  record_ids_.emplace_back(record_id);
  record_versions_.emplace_back(record_version);
}

ShreddedRecordList ShreddedRecordListBuilder::get() {
  return ShreddedRecordList(
      std::move(record_ids_),
      std::move(record_versions_),
      std::move(columns_));
}

} // namespace eventql

