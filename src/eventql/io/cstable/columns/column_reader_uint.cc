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
#include <eventql/io/cstable/columns/column_reader_uint.h>
#include <eventql/io/cstable/columns/page_reader_uint32.h>
#include <eventql/io/cstable/columns/page_reader_uint64.h>
#include <eventql/io/cstable/columns/page_reader_leb128.h>
#include <eventql/io/cstable/columns/page_reader_bitpacked.h>
#include <eventql/io/cstable/ColumnWriter.h>

#include "eventql/eventql.h"

namespace cstable {

UnsignedIntColumnReader::UnsignedIntColumnReader(
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

    case ColumnEncoding::UINT64_PLAIN:
      data_reader_ = mkScoped(new UInt64PageReader(key, page_mgr));
      break;

    case ColumnEncoding::UINT64_LEB128:
      data_reader_ = mkScoped(new LEB128PageReader(key, page_mgr));
      break;

    case ColumnEncoding::UINT32_PLAIN:
      data_reader_ = mkScoped(new UInt32PageReader(key, page_mgr));
      break;

    case ColumnEncoding::UINT32_BITPACKED:
    case ColumnEncoding::BOOLEAN_BITPACKED:
      data_reader_ = mkScoped(new BitPackedIntPageReader(key, page_mgr));
      break;

    default:
      RAISEF(
          kIllegalArgumentError,
          "invalid storage type for unsigned integer column '$0'",
          config_.column_name);

  }
}

bool UnsignedIntColumnReader::readBoolean(
    uint64_t* rlvl,
    uint64_t* dlvl,
    bool* value) {
  uint64_t tmp;
  if (readUnsignedInt(rlvl, dlvl, &tmp)) {
    *value = tmp > 0;
    return true;
  } else {
    *value = false;
    return false;
  }
}

bool UnsignedIntColumnReader::readUnsignedInt(
    uint64_t* rlvl,
    uint64_t* dlvl,
    uint64_t* value) {
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

  if (*dlvl == config_.dlevel_max) {
    *value = data_reader_->readUnsignedInt();
    return true;
  } else {
    *value = 0;
    return false;
  }
}

bool UnsignedIntColumnReader::readSignedInt(
    uint64_t* rlvl,
    uint64_t* dlvl,
    int64_t* value) {
  uint64_t tmp;
  if (readUnsignedInt(rlvl, dlvl, &tmp)) {
    *value = tmp;
    return true;
  } else {
    *value = 0;
    return false;
  }
}

bool UnsignedIntColumnReader::readFloat(
    uint64_t* rlvl,
    uint64_t* dlvl,
    double* value) {
  uint64_t tmp;
  if (readUnsignedInt(rlvl, dlvl, &tmp)) {
    *value = tmp;
    return true;
  } else {
    *value = 0;
    return false;
  }
}

bool UnsignedIntColumnReader::readString(
    uint64_t* rlvl,
    uint64_t* dlvl,
    String* value) {
  uint64_t tmp;
  if (readUnsignedInt(rlvl, dlvl, &tmp)) {
    *value = StringUtil::toString(tmp);
    return true;
  } else {
    *value = "";
    return false;
  }
}

void UnsignedIntColumnReader::skipValue() {
  uint64_t rlvl;
  uint64_t dlvl;
  uint64_t val;
  readUnsignedInt(&rlvl, &dlvl, &val);
}

void UnsignedIntColumnReader::copyValue(ColumnWriter* writer) {
  uint64_t rlvl;
  uint64_t dlvl;
  uint64_t val;
  readUnsignedInt(&rlvl, &dlvl, &val);
  if (dlvl == config_.dlevel_max) {
    writer->writeUnsignedInt(rlvl, dlvl, val);
  } else {
    writer->writeNull(rlvl, dlvl);
  }
}

void UnsignedIntColumnReader::rewind() {
  if (rlevel_reader_) {
    rlevel_reader_->rewind();
  }

  if (dlevel_reader_) {
    dlevel_reader_->rewind();
  }

  data_reader_->rewind();
}

} // namespace cstable

