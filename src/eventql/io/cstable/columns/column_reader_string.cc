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
#include <eventql/io/cstable/ColumnWriter.h>
#include <eventql/io/cstable/columns/column_reader_string.h>
#include <eventql/io/cstable/columns/page_reader_lenencstring.h>

namespace cstable {

StringColumnReader::StringColumnReader(
    ColumnConfig config,
    ScopedPtr<UnsignedIntPageReader> rlevel_reader,
    ScopedPtr<UnsignedIntPageReader> dlevel_reader,
    const PageManager* page_mgr) :
    DefaultColumnReader(
        config,
        std::move(rlevel_reader),
        std::move(dlevel_reader)) {
  PageIndexKey key = {
    .column_id = config.column_id,
    .entry_type = PageIndexEntryType::DATA
  };

  switch (config_.storage_type) {

    case ColumnEncoding::STRING_PLAIN:
      data_reader_ = mkScoped(new LenencStringPageReader(key, page_mgr));
      break;

    default:
      RAISEF(
          kIllegalArgumentError,
          "invalid storage type for string column '$0'",
          config_.column_name);

  }
}

bool StringColumnReader::readBoolean(
    uint64_t* rlvl,
    uint64_t* dlvl,
    bool* value) {
  String tmp;
  if (readString(rlvl, dlvl, &tmp)) {
    *value = (tmp == "true");
    return true;
  } else {
    *value = false;
    return false;
  }
}

bool StringColumnReader::readUnsignedInt(
    uint64_t* rlvl,
    uint64_t* dlvl,
    uint64_t* value) {
  String tmp;
  if (readString(rlvl, dlvl, &tmp)) {
    try {
      *value = std::stoull(tmp);
    } catch (...) {
      RAISEF(kIllegalArgumentError, "can't convert '$0' to uint", tmp);
    }

    return true;
  } else {
    *value = 0;
    return false;
  }
}

bool StringColumnReader::readSignedInt(
    uint64_t* rlvl,
    uint64_t* dlvl,
    int64_t* value) {
  String tmp;
  if (readString(rlvl, dlvl, &tmp)) {
    *value = std::stoll(tmp);
    return true;
  } else {
    *value = 0;
    return false;
  }
}

bool StringColumnReader::readFloat(
    uint64_t* rlvl,
    uint64_t* dlvl,
    double* value) {
  String tmp;
  if (readString(rlvl, dlvl, &tmp)) {
    *value = std::stod(tmp);
    return true;
  } else {
    *value = 0;
    return false;
  }
}

bool StringColumnReader::readString(
    uint64_t* rlvl,
    uint64_t* dlvl,
    String* value) {
  if (rlevel_reader_) {
    *rlvl = rlevel_reader_->readUnsignedInt();
  } else {
    *rlvl = 0;
  }

  if (dlevel_reader_) {
    *dlvl = dlevel_reader_->readUnsignedInt();
  } else {
    *dlvl = 0;
  }

  value->clear();

  if (*dlvl == config_.dlevel_max) {
    data_reader_->readString(value);
    return true;
  } else {
    return false;
  }
}

void StringColumnReader::skipValue() {
  uint64_t rlvl;
  uint64_t dlvl;
  String val;
  readString(&rlvl, &dlvl, &val);
}

void StringColumnReader::copyValue(ColumnWriter* writer) {
  uint64_t rlvl;
  uint64_t dlvl;
  String val;
  readString(&rlvl, &dlvl, &val);
  if (dlvl == config_.dlevel_max) {
    writer->writeString(rlvl, dlvl, val);
  } else {
    writer->writeNull(rlvl, dlvl);
  }
}

void StringColumnReader::rewind() {
  if (rlevel_reader_) {
    rlevel_reader_->rewind();
  }

  if (dlevel_reader_) {
    dlevel_reader_->rewind();
  }

  data_reader_->rewind();
}

} // namespace cstable

