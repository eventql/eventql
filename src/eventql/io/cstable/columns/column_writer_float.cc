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
#include <eventql/io/cstable/columns/column_writer_float.h>
#include <eventql/io/cstable/columns/page_writer_ieee754.h>

#include "eventql/eventql.h"

namespace cstable {

FloatColumnWriter::FloatColumnWriter(
    ColumnConfig config,
    PageManager* page_mgr) :
    DefaultColumnWriter(config, page_mgr) {
  PageIndexKey key = {
    .column_id = config.column_id,
    .entry_type = PageIndexEntryType::DATA
  };

  switch (config_.storage_type) {

    case ColumnEncoding::FLOAT_IEEE754:
      data_writer_ = mkScoped(new IEEE754PageWriter(key, page_mgr));
      break;

    default:
      RAISEF(
          kIllegalArgumentError,
          "invalid storage type for float column '$0'",
          config_.column_name);

  }
}

void FloatColumnWriter::writeBoolean(
    uint64_t rlvl,
    uint64_t dlvl,
    bool value) {
  writeFloat(rlvl, dlvl, value ? 1 : 0);
}

void FloatColumnWriter::writeUnsignedInt(
    uint64_t rlvl,
    uint64_t dlvl,
    uint64_t value) {
  writeFloat(rlvl, dlvl, value);
}

void FloatColumnWriter::writeSignedInt(
    uint64_t rlvl,
    uint64_t dlvl,
    int64_t value) {
  writeFloat(rlvl, dlvl, value);
}

void FloatColumnWriter::writeFloat(
    uint64_t rlvl,
    uint64_t dlvl,
    double value) {
  if (rlevel_writer_.get()) {
    rlevel_writer_->appendValue(rlvl);
  }

  if (dlevel_writer_.get()) {
    dlevel_writer_->appendValue(dlvl);
  }

  data_writer_->appendValue(value);
}

void FloatColumnWriter::writeString(
    uint64_t rlvl,
    uint64_t dlvl,
    const char* data,
    size_t size) {
  double value = std::stod(String(data, size));
  writeFloat(rlvl, dlvl, value);
}

void FloatColumnWriter::writeDateTime(
    uint64_t rlvl,
    uint64_t dlvl,
    UnixTime time) {
  writeFloat(rlvl, dlvl, time.unixMicros());
}

} // namespace cstable

