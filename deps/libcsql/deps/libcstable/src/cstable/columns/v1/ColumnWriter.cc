/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <cstable/columns/v1/ColumnWriter.h>
#include <stx/ieee754.h>

using namespace stx;

namespace cstable {
namespace v1 {

ColumnWriter::ColumnWriter(
    size_t r_max,
    size_t d_max) :
    cstable::ColumnWriter(r_max, d_max),
    rlvl_writer_(r_max),
    dlvl_writer_(d_max),
    num_vals_(0) {}

void ColumnWriter::commit() {
  rlvl_writer_.flush();
  dlvl_writer_.flush();
}

void ColumnWriter::write(void* buf, size_t buf_len) {
  stx::util::BinaryMessageWriter writer(buf, buf_len);
  writer.appendUInt64(num_vals_);
  writer.appendUInt64(rlvl_writer_.size());
  writer.appendUInt64(dlvl_writer_.size());
  writer.appendUInt64(size());
  writer.append(rlvl_writer_.data(), rlvl_writer_.size());
  writer.append(dlvl_writer_.data(), dlvl_writer_.size());
  write(&writer);
}

size_t ColumnWriter::bodySize() const {
  return 32 + rlvl_writer_.size() + dlvl_writer_.size() + size();
}

void ColumnWriter::writeNull(uint64_t rlvl, uint64_t dlvl) {
  rlvl_writer_.encode(rlvl);
  dlvl_writer_.encode(dlvl);
  ++num_vals_;
}

void ColumnWriter::writeBoolean(
    uint64_t rlvl,
    uint64_t dlvl,
    bool value) {
  uint64_t v = value ? 1 : 0;
  addDatum(rlvl, dlvl, &v, sizeof(v));
}

void ColumnWriter::writeUnsignedInt(
    uint64_t rlvl,
    uint64_t dlvl,
    uint64_t value) {
  addDatum(rlvl, dlvl, &value, sizeof(value));
}

void ColumnWriter::writeSignedInt(
    uint64_t rlvl,
    uint64_t dlvl,
    int64_t value) {
  addDatum(rlvl, dlvl, &value, sizeof(value));
}

void ColumnWriter::writeFloat(
    uint64_t rlvl,
    uint64_t dlvl,
    double value) {
  uint64_t val = IEEE754::toBytes(value);
  addDatum(rlvl, dlvl, &val, sizeof(val));
}

void ColumnWriter::writeString(
    uint64_t rlvl,
    uint64_t dlvl,
    const char* data,
    size_t size) {
  addDatum(rlvl, dlvl, data, size);
}


void ColumnWriter::writeDateTime(
    uint64_t rlvl,
    uint64_t dlvl,
    UnixTime value) {
  uint64_t val = value.unixMicros();
  addDatum(rlvl, dlvl, &val, sizeof(val));
}

} // namespace v1
} // namespace cstable

