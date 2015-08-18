/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <cstable/StringColumnWriter.h>

namespace stx {
namespace cstable {

StringColumnWriter::StringColumnWriter(
    uint64_t r_max,
    uint64_t d_max,
    size_t max_strlen) :
    ColumnWriter(r_max, d_max) {}

void StringColumnWriter::addDatum(
    uint64_t rep_level,
    uint64_t def_level,
    const void* data,
    size_t size) {
  addDatum(rep_level, def_level, String((const char*) data, size));
}

void StringColumnWriter::addDatum(
    uint64_t rep_level,
    uint64_t def_level,
    const String& value) {
  rlvl_writer_.encode(rep_level);
  dlvl_writer_.encode(def_level);
  data_writer_.appendUInt32(value.size()); // FIXPAUL use varuint...
  data_writer_.append(value.data(), value.size());
  ++num_vals_;
}

void StringColumnWriter::write(util::BinaryMessageWriter* writer) {
  writer->append(data_writer_.data(), data_writer_.size());
}

size_t StringColumnWriter::size() const {
  return data_writer_.size();
}


} // namespace cstable
} // namespace stx
