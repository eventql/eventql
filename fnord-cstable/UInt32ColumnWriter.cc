/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-cstable/UInt32ColumnWriter.h>

namespace fnord {
namespace cstable {

UInt32ColumnWriter::UInt32ColumnWriter(
    uint64_t r_max,
    uint64_t d_max) :
    ColumnWriter(r_max, d_max) {}

void UInt32ColumnWriter::addDatum(
    uint64_t rep_level,
    uint64_t def_level,
    uint16_t value) {
  rlvl_writer_.encode(rep_level);
  dlvl_writer_.encode(def_level);
  data_writer_.appendUInt32(value);
}

void UInt32ColumnWriter::addNull(
    uint64_t rep_level,
    uint64_t def_level) {
  rlvl_writer_.encode(rep_level);
  dlvl_writer_.encode(def_level);
}

void UInt32ColumnWriter::commit() {
  rlvl_writer_.flush();
}

void UInt32ColumnWriter::write(util::BinaryMessageWriter* writer) {
  writer->append(data_writer_.data(), data_writer_.size());
}

size_t UInt32ColumnWriter::size() const {
  return sizeof(uint32_t) + data_writer_.size();
}


} // namespace cstable
} // namespace fnord
