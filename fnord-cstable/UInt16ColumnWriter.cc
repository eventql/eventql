/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-cstable/UInt16ColumnWriter.h>

namespace fnord {
namespace cstable {

UInt16ColumnWriter::UInt16ColumnWriter(
    uint64_t r_max,
    uint64_t d_max) :
    ColumnWriter(r_max, d_max) {}

void UInt16ColumnWriter::addDatum(
    uint64_t rep_level,
    uint64_t def_level,
    uint16_t value) {
  rlvl_writer_.encode(rep_level);
  dlvl_writer_.appendUInt8(def_level);
  data_writer_.appendUInt16(value);
}

void UInt16ColumnWriter::addNull(
    uint64_t rep_level,
    uint64_t def_level) {
  rlvl_writer_.encode(rep_level);
  dlvl_writer_.appendUInt8(def_level);
}

void UInt16ColumnWriter::write(void* buf, size_t buf_len) {
  util::BinaryMessageWriter writer(buf, buf_len);
  writer.appendUInt64(rlvl_writer_.size());
  writer.appendUInt64(dlvl_writer_.size());
  writer.appendUInt64(data_writer_.size());
  writer.append(rlvl_writer_.data(), rlvl_writer_.size());
  writer.append(dlvl_writer_.data(), dlvl_writer_.size());
  writer.append(data_writer_.data(), data_writer_.size());
}

size_t UInt16ColumnWriter::bodySize() const {
  return 24 + data_writer_.size() + rlvl_writer_.size() + dlvl_writer_.size();
}


} // namespace cstable
} // namespace fnord
