/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-cstable/UInt16ColumnReader.h>

namespace fnord {
namespace cstable {

UInt16ColumnReader::UInt16ColumnReader(
    uint64_t r_max,
    uint64_t d_max,
    void* data,
    size_t size) :
    ColumnReader<
        util::PFORDecoder,
        util::PFORDecoder,
        util::PFORDecoder>(r_max, d_max, data, size) {}

bool UInt16ColumnReader::next(
    uint64_t* rep_level,
    uint64_t* def_level,
    uint16_t* data) {
  auto r = rlvl_reader_.next();
  auto d = dlvl_reader_.next();

  *rep_level = r;
  *def_level = d;

  if (d == d_max_) {
    *data = data_reader_.next();
    return true;
  } else {
    return false;
  }
}

} // namespace cstable
} // namespace fnord
