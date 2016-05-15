/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#include <eventql/infra/cstable/columns/v1/ColumnWriter.h>
#include <eventql/util/ieee754.h>

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

