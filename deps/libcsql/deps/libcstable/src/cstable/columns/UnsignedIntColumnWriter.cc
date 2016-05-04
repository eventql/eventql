/**
 * This file is part of the "libcstable" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * libcstable is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <cstable/columns/UnsignedIntColumnWriter.h>

using namespace stx;

namespace cstable {

UnsignedIntColumnWriter::UnsignedIntColumnWriter(
    ColumnConfig config,
    RefPtr<PageManager> page_mgr,
    RefPtr<PageIndex> page_idx) :
    DefaultColumnWriter(config, page_mgr, page_idx) {
  PageIndexKey key = {
    .column_id = config.column_id,
    .entry_type = PageIndexEntryType::DATA
  };

  switch (config_.storage_type) {

    case ColumnEncoding::UINT64_PLAIN:
      data_writer_ = mkScoped(new UInt64PageWriter(key, page_mgr, page_idx));
      break;

    default:
      RAISEF(
          kIllegalArgumentError,
          "invalid storage type for unsigned integer column '$0'",
          config_.column_name);

  }
}

void UnsignedIntColumnWriter::writeBoolean(
    uint64_t rlvl,
    uint64_t dlvl,
    bool value) {
  writeUnsignedInt(rlvl, dlvl, value ? 1 : 0);
}

void UnsignedIntColumnWriter::writeUnsignedInt(
    uint64_t rlvl,
    uint64_t dlvl,
    uint64_t value) {
  if (rlevel_writer_.get()) {
    rlevel_writer_->appendValue(rlvl);
  }

  if (dlevel_writer_.get()) {
    dlevel_writer_->appendValue(dlvl);
  }

  data_writer_->appendValue(value);
}

void UnsignedIntColumnWriter::writeSignedInt(
    uint64_t rlvl,
    uint64_t dlvl,
    int64_t value) {
  if (value < 0) {
    value = 0;
  }

  writeUnsignedInt(rlvl, dlvl, (uint64_t) value);
}


void UnsignedIntColumnWriter::writeFloat(
    uint64_t rlvl,
    uint64_t dlvl,
    double value) {
  if (value < 0) {
    value = 0;
  }

  writeUnsignedInt(rlvl, dlvl, value);
}

void UnsignedIntColumnWriter::writeString(
    uint64_t rlvl,
    uint64_t dlvl,
    const char* data,
    size_t size) {
  uint64_t value = std::stoull(String(data, size));
  writeUnsignedInt(rlvl, dlvl, value);
}

void UnsignedIntColumnWriter::writeDateTime(
    uint64_t rlvl,
    uint64_t dlvl,
    UnixTime time) {
  writeUnsignedInt(rlvl, dlvl, time.unixMicros());
}

} // namespace cstable

