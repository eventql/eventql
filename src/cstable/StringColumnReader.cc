/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <cstable/StringColumnReader.h>

namespace stx {
namespace cstable {

StringColumnReader::StringColumnReader(
    uint64_t r_max,
    uint64_t d_max,
    void* data,
    size_t size) :
    ColumnReader(r_max, d_max, data, size),
    data_reader_(data_, data_size_) {}

bool StringColumnReader::next(
    uint64_t* rep_level,
    uint64_t* def_level,
    void** data,
    size_t* data_len) {
  return next(
      rep_level,
      def_level,
      reinterpret_cast<const char**>(const_cast<const void**>(data)),
      data_len);
}

bool StringColumnReader::next(
    uint64_t* rep_level,
    uint64_t* def_level,
    const char** data,
    size_t* data_len) {
  auto r = rlvl_reader_.next();
  auto d = dlvl_reader_.next();

  *rep_level = r;
  *def_level = d;
  ++vals_read_;

  if (d == d_max_) {
    *data_len = *data_reader_.readUInt32();
    *data = data_reader_.readString(*data_len);
    return true;
  } else {
    *data_len = 0;
    return false;
  }
}

} // namespace cstable
} // namespace stx
