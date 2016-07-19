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
    record_versions_(std::move(record_versions)) {
  columns_.reserve(columns.size());
  for (auto& c : columns) {
    columns_.emplace_back(std::move(c));
  }
}

size_t ShreddedRecordList::getNumColumns() const {
  return columns_.size();
}

size_t ShreddedRecordList::getNumRecords() const {
  return record_ids_.size();
}

const SHA1Hash& ShreddedRecordList::getRecordID(size_t idx) const {
  return record_ids_[idx];
}

uint64_t ShreddedRecordList::getRecordVersion(size_t idx) const {
  return record_versions_[idx];
}

const ShreddedRecordColumn* ShreddedRecordList::getColumn(size_t idx) const {
  return &columns_[idx];
}

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

  columns_.resize(ncols);
  for (uint64_t j = 0; j < ncols; ++j) {
    auto& col = columns_[j];
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

void ShreddedRecordList::debugPrint() const {
  iputs("== record list; nrecs=$0 ncols=$1", record_ids_.size(), columns_.size());

  for (const auto& c : columns_) {
    iputs("== column: $0 ($1 vals)", c.column_name, c.values.size());

    for (const auto& v : c.values) {
      iputs("  >> r=$0 d=$1 v=$2", v.rlvl, v.dlvl, v.value);
    }
  }
}

ShreddedRecordListBuilder::ShreddedRecordListBuilder() {}

ShreddedRecordColumn* ShreddedRecordListBuilder::getColumn(
    const String& column_name) {
  auto& col_entry = columns_by_name_[column_name];
  if (!col_entry) {
    columns_.emplace_back();
    auto col = &columns_.back();
    col->column_name = column_name;
    col_entry = col;
  }

  return col_entry;
}

void ShreddedRecordListBuilder::addRecord(
    const SHA1Hash& record_id,
    uint64_t record_version) {
  record_ids_.emplace_back(record_id);
  record_versions_.emplace_back(record_version);
}

void ShreddedRecordListBuilder::addRecordFromProtobuf(
    const SHA1Hash& record_id,
    uint64_t record_version,
    const msg::DynamicMessage& msg) {
  addRecordFromProtobuf(record_id, record_version, msg.data(), *msg.schema());
}

void writeProtoNull(
    uint32_t r,
    uint32_t d,
    const String& column,
    const msg::MessageSchemaField& field,
    ShreddedRecordListBuilder* writer) {
  switch (field.type) {

    case msg::FieldType::OBJECT:
      for (const auto& f : field.schema->fields()) {
        writeProtoNull(r, d, column + "." + f.name, f, writer);
      }

      break;

    default:
      auto col = writer->getColumn(column);
      col->addNull(r, d);
      break;

  }
}

void writeProtoField(
    uint32_t r,
    uint32_t d,
    const msg::MessageObject& msg,
    const String& column,
    const msg::MessageSchemaField& field,
    ShreddedRecordListBuilder* writer) {
  auto col = writer->getColumn(column);

  switch (field.type) {

    case msg::FieldType::STRING: {
      col->addValue(r, d, msg.asString());
      break;
    }

    case msg::FieldType::UINT64: {
      col->addValue(r, d, StringUtil::toString(msg.asUInt64()));
      break;
    }

    case msg::FieldType::UINT32: {
      col->addValue(r, d, StringUtil::toString(msg.asUInt32()));
      break;
    }

    case msg::FieldType::DATETIME: {
      col->addValue(r, d, StringUtil::toString(msg.asUInt64()));
      break;
    }

    case msg::FieldType::DOUBLE: {
      col->addValue(r, d, StringUtil::toString(msg.asDouble()));
      break;
    }

    case msg::FieldType::BOOLEAN: {
      col->addValue(r, d, StringUtil::toString(msg.asBool() ? 1 : 0));
      break;
    }

    case msg::FieldType::OBJECT:
      RAISE(kIllegalStateError);

  }
}

static void addProtoRecordField(
    uint32_t r,
    uint32_t rmax,
    uint32_t d,
    const msg::MessageObject& msg,
    const msg::MessageSchema& msg_schema,
    const String& column,
    const msg::MessageSchemaField& field,
    ShreddedRecordListBuilder* writer) {
  auto next_r = r;
  auto next_d = d;

  if (field.repeated) {
    ++rmax;
  }

  if (field.optional || field.repeated) {
    ++next_d;
  }

  size_t n = 0;
  auto field_id = msg_schema.fieldId(field.name);
  for (const auto& o : msg.asObject()) {
    if (o.id != field_id) { // FIXME
      continue;
    }

    ++n;

    switch (field.type) {
      case msg::FieldType::OBJECT: {
        for (const auto& f : field.schema->fields()) {
          addProtoRecordField(
              next_r,
              rmax,
              next_d,
              o,
              *field.schema,
              column + field.name + ".",
              f,
              writer);
        }
        break;
      }

      default:
        writeProtoField(next_r, next_d, o, column + field.name, field, writer);
        break;
    }

    next_r = rmax;
  }

  if (n == 0) {
    if (!(field.optional || field.repeated)) {
      RAISEF(kIllegalArgumentError, "missing field: $0", column + field.name);
    }

    writeProtoNull(r, d, column + field.name, field, writer);
    return;
  }
}

void ShreddedRecordListBuilder::addRecordFromProtobuf(
    const SHA1Hash& record_id,
    uint64_t record_version,
    const msg::MessageObject& msg,
    const msg::MessageSchema& schema) {
  for (const auto& f : schema.fields()) {
    addProtoRecordField(0, 0, 0, msg, schema, "", f, this);
  }

  addRecord(record_id, record_version);
}

ShreddedRecordList ShreddedRecordListBuilder::get() {
  return ShreddedRecordList(
      std::move(record_ids_),
      std::move(record_versions_),
      std::move(columns_));
}

} // namespace eventql

