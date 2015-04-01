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
  writer_.appendUInt8(rep_level);
  //writer_.appendUInt8(def_level);
  writer_.appendUInt16(value);
}

void UInt16ColumnWriter::write(void** data, size_t* size) {
  *data = writer_.data();
  *size = writer_.size();
}


} // namespace cstable
} // namespace fnord
