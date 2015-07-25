/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <cstable/BitPackedIntColumnWriter.h>

namespace stx {
namespace cstable {

BitPackedIntColumnWriter::BitPackedIntColumnWriter(
    uint64_t r_max,
    uint64_t d_max,
    uint32_t max_value /* = 0xffffffff */) :
    ColumnWriter(r_max, d_max),
    max_value_(max_value),
    data_writer_(max_value) {}

void BitPackedIntColumnWriter::addDatum(
    uint64_t rep_level,
    uint64_t def_level,
    const void* data,
    size_t size) {
  if (size != sizeof(uint32_t)) {
    RAISE(kIllegalArgumentError, "size != sizeof(uint32_t)");
  }

  addDatum(rep_level, def_level, *((const uint32_t*) data));
}

void BitPackedIntColumnWriter::addDatum(
    uint64_t rep_level,
    uint64_t def_level,
    uint32_t value) {
  rlvl_writer_.encode(rep_level);
  dlvl_writer_.encode(def_level);
  data_writer_.encode(value);
  ++num_vals_;
}

void BitPackedIntColumnWriter::commit() {
  ColumnWriter::commit();
  data_writer_.flush();
}

void BitPackedIntColumnWriter::write(util::BinaryMessageWriter* writer) {
  writer->appendUInt32(max_value_);
  writer->append(data_writer_.data(), data_writer_.size());
}

size_t BitPackedIntColumnWriter::size() const {
  return sizeof(uint32_t) + data_writer_.size();
}


} // namespace cstable
} // namespace stx
