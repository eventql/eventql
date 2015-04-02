/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_CSTABLE_COLUMNREADER_H
#define _FNORD_CSTABLE_COLUMNREADER_H
#include <fnord-base/stdtypes.h>
#include <fnord-base/util/binarymessagereader.h>
#include <fnord-base/util/RLEDecoder.h>

namespace fnord {
namespace cstable {

class ColumnReader {
public:

  ColumnReader(
      uint64_t r_max,
      uint64_t d_max,
      void* data,
      size_t size) :
      r_max_(r_max),
      d_max_(d_max),
      reader_(data, size),
      rlvl_size_(*reader_.readUInt64()),
      dlvl_size_(*reader_.readUInt64()),
      data_size_(*reader_.readUInt64()),
      rlvl_reader_(((char *) data) + 24, rlvl_size_, r_max),
      dlvl_reader_(((char *) data) + 24 + rlvl_size_, dlvl_size_, d_max),
      data_(((char *) data) + 24 + rlvl_size_ + dlvl_size_) {}

  virtual ~ColumnReader() {}

  virtual bool next(
      uint64_t* rep_level,
      uint64_t* def_level,
      void** data,
      size_t* data_len) = 0;

protected:
  uint64_t r_max_;
  uint64_t d_max_;
  util::BinaryMessageReader reader_;
  uint64_t rlvl_size_;
  uint64_t dlvl_size_;
  uint64_t data_size_;
  util::RLEDecoder rlvl_reader_;
  util::RLEDecoder dlvl_reader_;
  void* data_;
};

} // namespace cstable
} // namespace fnord

#endif
