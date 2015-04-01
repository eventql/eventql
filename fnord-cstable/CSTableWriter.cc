/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-cstable/CSTableWriter.h>
#include <fnord-cstable/BinaryFormat.h>

namespace fnord {
namespace cstable {

CSTableWriter::CSTableWriter(
    const String& filename,
    const uint64_t num_records) :
    file_(File::openFile(
        filename,
        File::O_READ | File::O_WRITE | File::O_CREATE)) {
  util::BinaryMessageWriter header;
  header.appendUInt32(BinaryFormat::kMagicBytes);
  header.appendUInt16(BinaryFormat::kVersion);
  header.appendUInt64(0); // flags
  header.appendUInt64(num_records);
  file_.write(header.data(), header.size());
  offset_ = header.size();
}

void CSTableWriter::addColumn(
    const String& column_name,
    ColumnWriter* column_writer) {
  void* col_data;
  size_t col_data_size;
  column_writer->write(&col_data, &col_data_size);

  ColumnInfo ci;
  ci.name = column_name;
  ci.body_offset = 0;
  ci.size = col_data_size;
  ci.writer = column_writer;

  columns_.emplace_back(ci);
}

void CSTableWriter::commit() {
  /* write header field: num_cols */
  uint32_t num_cols = columns_.size();
  file_.write(&num_cols, sizeof(num_cols));
  offset_ += sizeof(num_cols);

  /* calculate column start offsets */
  size_t col_offset = offset_;
  for (const auto& col : columns_) {
    col_offset += 36 + col.name.length();
  }

  for (auto& col : columns_) {
    col.body_offset = col_offset;
    col_offset += col.size;
  }

  /* write column headers */
  for (auto& col : columns_) {
    util::BinaryMessageWriter col_header;
    col_header.appendUInt32(col.name.length());
    col_header.append(col.name.data(), col.name.length());
    col_header.appendUInt64(col.writer->maxRepetitionLevel());
    col_header.appendUInt64(col.writer->maxDefinitionLevel());
    col_header.appendUInt64(col.body_offset);
    col_header.appendUInt64(col.size);
    file_.write(col_header.data(), col_header.size());
  }

  /* write column data */
  for (auto& col : columns_) {
    void* col_data;
    size_t col_data_size;
    col.writer->write(&col_data, &col_data_size);
    file_.write(col_data, col_data_size);
  }
}

} // namespace cstable
} // namespace fnord

