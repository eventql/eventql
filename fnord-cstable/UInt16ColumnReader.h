/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_CSTABLE_UINT16COLUMNREADER_H
#define _FNORD_CSTABLE_UINT16COLUMNREADER_H
#include <fnord-base/stdtypes.h>
#include <fnord-base/util/binarymessagereader.h>

namespace fnord {
namespace cstable {

class UInt16ColumnReader {
public:

  UInt16ColumnReader(
      uint64_t r_max,
      uint64_t d_max,
      void* data,
      size_t size);

  bool next(uint64_t* rep_level, uint64_t* def_level, uint16_t* data);

protected:
  uint64_t r_max_;
  uint64_t d_max_;
  util::BinaryMessageReader reader_;
  uint64_t rlvl_size_;
  uint64_t dlvl_size_;
  uint64_t data_size_;
  util::BinaryMessageReader rlvl_reader_;
  util::BinaryMessageReader dlvl_reader_;
  util::BinaryMessageReader data_reader_;
};

} // namespace cstable
} // namespace fnord

#endif
