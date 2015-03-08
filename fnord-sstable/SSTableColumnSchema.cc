/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-sstable/SSTableColumnSchema.h>
#include <fnord-sstable/SSTableWriter.h>
#include <fnord-base/util/BinaryMessageReader.h>
#include <fnord-base/util/BinaryMessageWriter.h>

namespace fnord {
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
}

SSTableColumnType SSTableColumnSchema::columnType(SSTableColumnID id) const {
  auto iter = col_info_.find(id);
  if (iter == col_info_.end()) {
    RAISEF(kIndexError, "invalid column index: $0", id);
  }

  return iter->second.type;
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

void SSTableColumnSchema::writeIndex(SSTableWriter* sstable_writer) {
  Buffer buf;
  writeIndex(&buf);

  sstable_writer->writeIndex(SSTableColumnSchema::kSSTableIndexID, buf);
}

} // namespace sstable
} // namespace fnord
