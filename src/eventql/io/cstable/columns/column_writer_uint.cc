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
#include "eventql/eventql.h"
#include <eventql/io/cstable/columns/column_writer_uint.h>
#include <eventql/io/cstable/columns/page_writer_uint32.h>
#include <eventql/io/cstable/columns/page_writer_uint64.h>
#include <eventql/io/cstable/columns/page_writer_leb128.h>
#include <eventql/io/cstable/columns/page_writer_bitpacked.h>

namespace cstable {

UnsignedIntColumnWriter::UnsignedIntColumnWriter(
    ColumnConfig config,
    PageManager* page_mgr) :
    DefaultColumnWriter(config, page_mgr) {
  PageIndexKey key = {
    .column_id = config.column_id,
    .entry_type = PageIndexEntryType::DATA
  };

  switch (config_.storage_type) {

    case ColumnEncoding::UINT64_PLAIN:
      data_writer_ = mkScoped(new UInt64PageWriter(key, page_mgr));
      break;

    case ColumnEncoding::UINT64_LEB128:
      data_writer_ = mkScoped(new LEB128PageWriter(key, page_mgr));
      break;

    case ColumnEncoding::UINT32_PLAIN:
      data_writer_ = mkScoped(new UInt32PageWriter(key, page_mgr));
      break;

    case ColumnEncoding::UINT32_BITPACKED:
      data_writer_ = mkScoped(new BitPackedIntPageWriter(key, page_mgr));
      break;

    case ColumnEncoding::BOOLEAN_BITPACKED:
      data_writer_ = mkScoped(new BitPackedIntPageWriter(key, page_mgr, 1));
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

void UnsignedIntColumnWriter::flush() {
  if (rlevel_writer_.get()) rlevel_writer_->flush();
  if (dlevel_writer_.get()) dlevel_writer_->flush();
  data_writer_->flush();
}

} // namespace cstable

