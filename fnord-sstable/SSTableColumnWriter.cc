/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-sstable/SSTableColumnWriter.h>

namespace fnord {
namespace sstable {

SSTableColumnWriter::SSTableColumnWriter(
    SSTableColumnSchema* schema) :
    schema_(schema) {}

void SSTableColumnWriter::addUINT32Column(SSTableColumnID id, uint32_t value) {
#ifndef FNORD_NODEBUG
  if (schema_->columnType(id) != SSTableColumnType::UINT32) {
    RAISEF(kIllegalArgumentError, "invalid column type for column_id: $0", id);
  }
#endif

  msg_writer_.appendUInt32(id);
  msg_writer_.appendUInt32(value);
}

void* SSTableColumnWriter::data() const {
  return msg_writer_.data();
}

size_t SSTableColumnWriter::size() const {
  return msg_writer_.size();
}

} // namespace sstable
} // namespace fnord
