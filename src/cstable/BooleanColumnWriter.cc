/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <cstable/BooleanColumnWriter.h>

namespace stx {
namespace cstable {

BooleanColumnWriter::BooleanColumnWriter(
    uint64_t r_max,
    uint64_t d_max) :
    ColumnWriter(r_max, d_max),
    data_writer_(1) {}

void BooleanColumnWriter::addDatum(
    uint64_t rep_level,
    uint64_t def_level,
    const void* data,
    size_t size) {
  if (size != sizeof(uint8_t)) {
    RAISE(kIllegalArgumentError, "size != sizeof(uint8_t)");
  }

  addDatum(rep_level, def_level, (*((const uint8_t*) data)) > 0);
}

void BooleanColumnWriter::addDatum(
    uint64_t rep_level,
    uint64_t def_level,
    bool value) {
  rlvl_writer_.encode(rep_level);
  dlvl_writer_.encode(def_level);
  data_writer_.encode(value);
  ++num_vals_;
}

void BooleanColumnWriter::commit() {
  ColumnWriter::commit();
  data_writer_.flush();
}

void BooleanColumnWriter::write(util::BinaryMessageWriter* writer) {
  writer->append(data_writer_.data(), data_writer_.size());
}

size_t BooleanColumnWriter::size() const {
  return data_writer_.size();
}


} // namespace cstable
} // namespace stx
