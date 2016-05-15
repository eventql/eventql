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
#include <eventql/util/ieee754.h>
#include <eventql/infra/sstable/SSTableColumnReader.h>

namespace util {
namespace sstable {

SSTableColumnReader::SSTableColumnReader(
    SSTableColumnSchema* schema,
    const Buffer& buf) :
    schema_(schema),
    buf_(buf),
    msg_reader_(buf_.data(), buf_.size()) {
  while (msg_reader_.remaining() > 0) {
    auto col_id = *msg_reader_.readUInt32();
    auto col_type = schema_->columnType(col_id);

    switch (col_type) {

      case SSTableColumnType::UINT32:
        col_data_.emplace_back(col_id, *msg_reader_.readUInt32(), 0);
        break;

      case SSTableColumnType::UINT64:
      case SSTableColumnType::FLOAT:
        col_data_.emplace_back(col_id, *msg_reader_.readUInt64(), 0);
        break;

      case SSTableColumnType::STRING: {
        uint32_t size = *msg_reader_.readUInt32();
        uint64_t data = (uint64_t) msg_reader_.read(size);
        col_data_.emplace_back(col_id, data, size);
        break;
      }

    }
  }
}

uint32_t SSTableColumnReader::getUInt32Column(SSTableColumnID id) {
#ifndef FNORD_NODEBUG
  if (schema_->columnType(id) != SSTableColumnType::UINT32) {
    RAISEF(kIllegalArgumentError, "invalid column type for column_id: $0", id);
  }
#endif

  for (const auto& col : col_data_) {
    if (std::get<0>(col) == id) {
      return std::get<1>(col);
    }
  }

  RAISEF(kIndexError, "no value for column: $0", id);
}

uint64_t SSTableColumnReader::getUInt64Column(SSTableColumnID id) {
#ifndef FNORD_NODEBUG
  if (schema_->columnType(id) != SSTableColumnType::UINT64) {
    RAISEF(kIllegalArgumentError, "invalid column type for column_id: $0", id);
  }
#endif

  for (const auto& col : col_data_) {
    if (std::get<0>(col) == id) {
      return std::get<1>(col);
    }
  }

  RAISEF(kIndexError, "no value for column: $0", id);
}

double SSTableColumnReader::getFloatColumn(SSTableColumnID id) {
#ifndef FNORD_NODEBUG
  if (schema_->columnType(id) != SSTableColumnType::FLOAT) {
    RAISEF(kIllegalArgumentError, "invalid column type for column_id: $0", id);
  }
#endif

  for (const auto& col : col_data_) {
    if (std::get<0>(col) == id) {
      return IEEE754::fromBytes(std::get<1>(col));
    }
  }

  RAISEF(kIndexError, "no value for column: $0", id);
}

String SSTableColumnReader::getStringColumn(SSTableColumnID id) {
  switch (schema_->columnType(id)) {
    case SSTableColumnType::UINT32:
      return StringUtil::toString(getUInt32Column(id));
    case SSTableColumnType::UINT64:
      return StringUtil::toString(getUInt64Column(id));
    case SSTableColumnType::FLOAT:
      return StringUtil::toString(getFloatColumn(id));
    case SSTableColumnType::STRING:
      break;
  }

  for (const auto& col : col_data_) {
    if (std::get<0>(col) == id) {
      return String((char*) std::get<1>(col), std::get<2>(col));
    }
  }

  RAISEF(kIndexError, "no value for column: $0", id);
}

Vector<String> SSTableColumnReader::getStringColumns(SSTableColumnID id) {
#ifndef FNORD_NODEBUG
  if (schema_->columnType(id) != SSTableColumnType::STRING) {
    RAISEF(kIllegalArgumentError, "invalid column type for column_id: $0", id);
  }
#endif

  Vector<String> data;
  for (const auto& col : col_data_) {
    if (std::get<0>(col) == id) {
      data.emplace_back((char*) std::get<1>(col), std::get<2>(col));
    }
  }

  return data;
}

} // namespace sstable
} // namespace util

