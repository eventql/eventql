/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-cstable/StringColumnWriter.h>

namespace fnord {
namespace cstable {

StringColumnWriter::StringColumnWriter(
    uint64_t r_max,
    uint64_t d_max,
    size_t max_strlen) :
    ColumnWriter(r_max, d_max),
    max_strlen_(max_strlen) {}

void StringColumnWriter::addDatum(
    uint64_t rep_level,
    uint64_t def_level,
    const String& value) {
  rlvl_writer_.encode(rep_level);
  dlvl_writer_.encode(def_level);
  data_writer_.appendUInt32(value.size());
  data_writer_.append(value.data(), value.size());
}

void StringColumnWriter::write(util::BinaryMessageWriter* writer) {
  writer->append(data_writer_.data(), data_writer_.size());
}

size_t StringColumnWriter::size() const {
  return data_writer_.size();
}


} // namespace cstable
} // namespace fnord
