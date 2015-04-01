/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-base/util/binarymessagereader.h>
#include <fnord-cstable/CSTableReader.h>
#include <fnord-cstable/BinaryFormat.h>

namespace fnord {
namespace cstable {

CSTableReader::CSTableReader(
    const String& filename) :
    file_(File::openFile(filename, File::O_READ)) {
  util::BinaryMessageReader header(file_.data(), file_.size());
  auto magic = *header.readUInt32();
  if (magic != BinaryFormat::kMagicBytes) {
    RAISE(kIllegalStateError, "not a valid cstable");
  }

  auto version = *header.readUInt16();
  if (version != BinaryFormat::kVersion) {
    RAISEF(kIllegalStateError, "unsupported sstable version: $0", version);
  }

  auto flags = *header.readUInt64();
  num_records_ = *header.readUInt64();
  num_columns_ = *header.readUInt32();

  fnord::iputs("num cols: $0, num records: $1", num_columns_, num_records_);

  for (int i = 0; i < num_columns_; ++i) {
    auto name_len = *header.readUInt32();
    auto name_data = (char *) header.read(name_len);

    auto rep_bits = *header.readUInt32();
    auto def_bits = *header.readUInt32();
    auto body_offset = *header.readUInt64();
    auto size = *header.readUInt64();

    ColumnInfo ci;
    ci.name = String(name_data, name_len);
    ci.body_offset = body_offset;
    ci.size = size;
    columns_.emplace(ci.name, ci);
  }
}

void CSTableReader::getColumn(
    const String& column_name,
    void** data,
    size_t* size) {
  auto col = columns_.find(column_name);
  if (col == columns_.end()) {
    RAISEF(kIndexError, "unknown column: $0", column_name);
  }

  *data = ((char *) file_.data()) + col->second.body_offset;
  *size = col->second.size;
}


} // namespace cstable
} // namespace fnord

