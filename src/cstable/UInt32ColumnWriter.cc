/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <cstable/UInt32ColumnWriter.h>

namespace stx {
namespace cstable {

UInt32ColumnWriter::UInt32ColumnWriter(
    uint64_t r_max,
    uint64_t d_max) :
    ColumnWriter(r_max, d_max) {}

void UInt32ColumnWriter::addDatum(
    uint64_t rep_level,
    uint64_t def_level,
    const void* data,
    size_t size) {
  if (size != sizeof(uint32_t)) {
    RAISE(kIllegalArgumentError, "size != sizeof(uint32_t)");
  }

  addDatum(rep_level, def_level, *((const uint32_t*) data));
}

void UInt32ColumnWriter::addDatum(
    uint64_t rep_level,
    uint64_t def_level,
    uint32_t value) {
  rlvl_writer_.encode(rep_level);
  dlvl_writer_.encode(def_level);
  data_writer_.appendUInt32(value);
  ++num_vals_;
}

void UInt32ColumnWriter::write(util::BinaryMessageWriter* writer) {
  writer->append(data_writer_.data(), data_writer_.size());
}

size_t UInt32ColumnWriter::size() const {
  return sizeof(uint32_t) + data_writer_.size();
}


} // namespace cstable
} // namespace stx
