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
#include <fnord-cstable/BooleanColumnReader.h>
#include <fnord-cstable/BitPackedIntColumnReader.h>

namespace fnord {
namespace cstable {

CSTableReader::CSTableReader(
    const String& filename) :
    CSTableReader(
        new io::MmappedFile(File::openFile(filename, File::O_READ))) {}

CSTableReader::CSTableReader(const RefPtr<VFSFile> file) : file_(file) {
  util::BinaryMessageReader header(file_->data(), file_->size());
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

  for (int i = 0; i < num_columns_; ++i) {
    auto type = *header.readUInt32();
    auto name_len = *header.readUInt32();
    auto name_data = (char *) header.read(name_len);

    auto r_max = *header.readUInt32();
    auto d_max = *header.readUInt32();
    auto body_offset = *header.readUInt64();
    auto size = *header.readUInt64();

    ColumnInfo ci;
    ci.type = (ColumnType) type;
    ci.name = String(name_data, name_len);
    ci.body_offset = body_offset;
    ci.size = size;
    ci.r_max = r_max;
    ci.d_max = d_max;
    columns_.emplace(ci.name, ci);
  }
}

RefPtr<ColumnReader> CSTableReader::getColumnReader(const String& column_name) {
  auto col = columns_.find(column_name);
  if (col == columns_.end()) {
    RAISEF(kIndexError, "unknown column: $0", column_name);
  }

  const auto& c = col->second;
  void* cdata = ((char *) file_->data()) + col->second.body_offset;

  switch (c.type) {
    case ColumnType::BOOLEAN:
      return new BooleanColumnReader(c.r_max, c.d_max, cdata, c.size);
    case ColumnType::UINT32_BITPACKED:
      return new BitPackedIntColumnReader(c.r_max, c.d_max, cdata, c.size);
    default:
      RAISEF(kIllegalStateError, "invalid column type: $0", (uint32_t) c.type);
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

  *data = ((char *) file_->data()) + col->second.body_offset;
  *size = col->second.size;
}

size_t CSTableReader::numRecords() const {
  return num_records_;
}

} // namespace cstable
} // namespace fnord

