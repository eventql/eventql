/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#include <eventql/io/cstable/columns/v1/LEB128ColumnWriter.h>


namespace cstable {
namespace v1 {

LEB128ColumnWriter::LEB128ColumnWriter(
    uint64_t r_max,
    uint64_t d_max,
    ColumnType type) :
    ColumnWriter(r_max, d_max),
    type_(type) {}

void LEB128ColumnWriter::addDatum(
    uint64_t rep_level,
    uint64_t def_level,
    const void* data,
    size_t size) {
  if (size != sizeof(uint32_t) && size != sizeof(uint64_t)) {
    RAISE(kIllegalArgumentError, "size != sizeof(uint32_t)");
  }

  uint64_t val =
      size == sizeof(uint32_t) ?
      *((const uint32_t*) data) :
      *((const uint64_t*) data);

  addDatum(rep_level, def_level, val);
}

void LEB128ColumnWriter::addDatum(
    uint64_t rep_level,
    uint64_t def_level,
    uint64_t value) {
  rlvl_writer_.encode(rep_level);
  dlvl_writer_.encode(def_level);
  data_writer_.appendVarUInt(value);
  ++num_vals_;
}

void LEB128ColumnWriter::write(util::BinaryMessageWriter* writer) {
  writer->append(data_writer_.data(), data_writer_.size());
}

size_t LEB128ColumnWriter::size() const {
  return data_writer_.size();
}

} // namespace v1
} // namespace cstable

