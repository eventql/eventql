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
#include <eventql/infra/sstable/SSTableColumnWriter.h>

namespace stx {
namespace sstable {

SSTableColumnWriter::SSTableColumnWriter(
    SSTableColumnSchema* schema) :
    schema_(schema) {}

void SSTableColumnWriter::addUInt32Column(SSTableColumnID id, uint32_t value) {
#ifndef FNORD_NODEBUG
  if (schema_->columnType(id) != SSTableColumnType::UINT32) {
    RAISEF(kIllegalArgumentError, "invalid column type for column_id: $0", id);
  }
#endif

  msg_writer_.appendUInt32(id);
  msg_writer_.appendUInt32(value);
}

void SSTableColumnWriter::addUInt64Column(SSTableColumnID id, uint64_t value) {
#ifndef FNORD_NODEBUG
  if (schema_->columnType(id) != SSTableColumnType::UINT64) {
    RAISEF(kIllegalArgumentError, "invalid column type for column_id: $0", id);
  }
#endif

  msg_writer_.appendUInt32(id);
  msg_writer_.appendUInt64(value);
}

void SSTableColumnWriter::addFloatColumn(SSTableColumnID id, double value) {
#ifndef FNORD_NODEBUG
  if (schema_->columnType(id) != SSTableColumnType::FLOAT) {
    RAISEF(kIllegalArgumentError, "invalid column type for column_id: $0", id);
  }
#endif

  msg_writer_.appendUInt32(id);
  msg_writer_.appendUInt64(IEEE754::toBytes(value));
}

void SSTableColumnWriter::addStringColumn(
    SSTableColumnID id,
    const String& value) {
#ifndef FNORD_NODEBUG
  if (schema_->columnType(id) != SSTableColumnType::STRING) {
    RAISEF(kIllegalArgumentError, "invalid column type for column_id: $0", id);
  }
#endif

  uint32_t len = value.length();
  msg_writer_.appendUInt32(id);
  msg_writer_.appendUInt32(len);
  msg_writer_.append(value.data(), len);
}

void* SSTableColumnWriter::data() const {
  return msg_writer_.data();
}

size_t SSTableColumnWriter::size() const {
  return msg_writer_.size();
}

} // namespace sstable
} // namespace stx
