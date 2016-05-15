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
#include <eventql/infra/sstable/SSTableColumnSchema.h>
#include <eventql/infra/sstable/sstablereader.h>
#include <eventql/infra/sstable/SSTableEditor.h>
#include <eventql/util/util/binarymessagereader.h>
#include <eventql/util/util/binarymessagewriter.h>

namespace util {
namespace sstable {

SSTableColumnSchema::SSTableColumnSchema() {}

void SSTableColumnSchema::addColumn(
    const String& name,
    uint32_t id,
    SSTableColumnType type) {
  SSTableColumnInfo info;
  info.name = name;
  info.type = type;
  col_info_[id] = info;
  col_ids_[name] = id;
}

SSTableColumnType SSTableColumnSchema::columnType(SSTableColumnID id) const {
  auto iter = col_info_.find(id);
  if (iter == col_info_.end()) {
    RAISEF(kIndexError, "invalid column index: $0", id);
  }

  return iter->second.type;
}

String SSTableColumnSchema::columnName(SSTableColumnID id) const {
  auto iter = col_info_.find(id);
  if (iter == col_info_.end()) {
    RAISEF(kIndexError, "invalid column index: $0", id);
  }

  return iter->second.name;
}

SSTableColumnID SSTableColumnSchema::columnID(const String& column_name) const {
  auto iter = col_ids_.find(column_name);
  if (iter == col_ids_.end()) {
    RAISEF(kIndexError, "invalid column: $0", column_name);
  }

  return iter->second;
}

Set<SSTableColumnID> SSTableColumnSchema::columnIDs() const {
  Set<SSTableColumnID> ids;

  for (const auto& c : col_info_) {
    ids.emplace(c.first);
  }

  return ids;
}

void SSTableColumnSchema::writeIndex(Buffer* buf) {
  util::BinaryMessageWriter writer;

  for (const auto& c : col_info_) {
    writer.appendUInt32((uint8_t) c.second.type);
    writer.appendUInt32(c.first);
    writer.appendUInt32(c.second.name.length());
    writer.append(c.second.name.data(), c.second.name.length());
  }

  buf->append(writer.data(), writer.size());
}

void SSTableColumnSchema::writeIndex(SSTableEditor* sstable_writer) {
  Buffer buf;
  writeIndex(&buf);

  sstable_writer->writeIndex(SSTableColumnSchema::kSSTableIndexID, buf);
}

void SSTableColumnSchema::loadIndex(const Buffer& buf) {
  util::BinaryMessageReader reader(buf.data(), buf.size());

  while (reader.remaining() > 0) {
    uint32_t col_type = *reader.readUInt32();
    uint32_t col_id = *reader.readUInt32();
    uint32_t col_name_len = *reader.readUInt32();
    String col_name((char*) reader.read(col_name_len), col_name_len);

    addColumn(col_name, col_id, (util::sstable::SSTableColumnType) col_type);
  }
}

void SSTableColumnSchema::loadIndex(
    SSTableReader* sstable_reader) {
  auto index = sstable_reader->readFooter(kSSTableIndexID);
  loadIndex(index);
}

} // namespace sstable
} // namespace util
