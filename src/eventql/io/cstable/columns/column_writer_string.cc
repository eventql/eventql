/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#include <eventql/io/cstable/columns/column_writer_string.h>
#include <eventql/io/cstable/columns/page_writer_lenencstring.h>

namespace cstable {

StringColumnWriter::StringColumnWriter(
    ColumnConfig config,
    PageManager* page_mgr) :
    DefaultColumnWriter(config, page_mgr) {
  PageIndexKey key = {
    .column_id = config.column_id,
    .entry_type = PageIndexEntryType::DATA
  };

  switch (config_.storage_type) {

    case ColumnEncoding::STRING_PLAIN:
      data_writer_ = mkScoped(new LenencStringPageWriter(key, page_mgr));
      break;

    default:
      RAISEF(
          kIllegalArgumentError,
          "invalid storage type for string column '$0'",
          config_.column_name);

  }
}

void StringColumnWriter::writeBoolean(
    uint64_t rlvl,
    uint64_t dlvl,
    bool value) {
  ColumnWriter::writeString(rlvl, dlvl, value ? "true" : "false");
}

void StringColumnWriter::writeUnsignedInt(
    uint64_t rlvl,
    uint64_t dlvl,
    uint64_t value) {
  ColumnWriter::writeString(rlvl, dlvl, StringUtil::toString(value));
}

void StringColumnWriter::writeSignedInt(
    uint64_t rlvl,
    uint64_t dlvl,
    int64_t value) {
  ColumnWriter::writeString(rlvl, dlvl, StringUtil::toString(value));
}

void StringColumnWriter::writeFloat(
    uint64_t rlvl,
    uint64_t dlvl,
    double value) {
  ColumnWriter::writeString(rlvl, dlvl, StringUtil::toString(value));
}

void StringColumnWriter::writeString(
    uint64_t rlvl,
    uint64_t dlvl,
    const char* data,
    size_t size) {
  if (rlevel_writer_.get()) {
    rlevel_writer_->appendValue(rlvl);
  }

  if (dlevel_writer_.get()) {
    dlevel_writer_->appendValue(dlvl);
  }

  data_writer_->appendValue(data, size);
}

void StringColumnWriter::writeDateTime(
    uint64_t rlvl,
    uint64_t dlvl,
    UnixTime time) {
  writeUnsignedInt(rlvl, dlvl, time.unixMicros());
}

} // namespace cstable

