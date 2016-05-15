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
#include <eventql/io/cstable/columns/v1/BitPackedIntColumnWriter.h>

#include "eventql/eventql.h"

namespace cstable {
namespace v1 {

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
  switch (size) {
    case sizeof(uint8_t):
      addDatum(rep_level, def_level, *((const uint8_t*) data));
      return;
    case sizeof(uint32_t):
      addDatum(rep_level, def_level, *((const uint32_t*) data));
      return;
    case sizeof(uint64_t):
      addDatum(rep_level, def_level, *((const uint64_t*) data));
      return;
  }

  RAISE(kIllegalArgumentError, "invalid size");
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


} // namespace v1
} // namespace cstable

