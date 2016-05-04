/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_CSTABLE_COLUMNREADER_H
#define _FNORD_CSTABLE_COLUMNREADER_H
#include <cstable/ColumnReader.h>
#include <stx/util/binarymessagereader.h>
#include <stx/util/BitPackDecoder.h>


namespace cstable {
namespace v1 {

class ColumnReader : public cstable::ColumnReader {
public:

  ColumnReader(
      uint64_t r_max,
      uint64_t d_max,
      void* data,
      size_t size) :
      r_max_(r_max),
      d_max_(d_max),
      reader_(data, size),
      vals_total_(*reader_.readUInt64()),
      vals_read_(0),
      rlvl_size_(*reader_.readUInt64()),
      dlvl_size_(*reader_.readUInt64()),
      data_size_(*reader_.readUInt64()),
      rlvl_reader_(((char *) data) + 32, rlvl_size_, r_max),
      dlvl_reader_(((char *) data) + 32 + rlvl_size_, dlvl_size_, d_max),
      data_(((char *) data) + 32 + rlvl_size_ + dlvl_size_) {}

  virtual ~ColumnReader() {}

  uint64_t maxRepetitionLevel() const override { return r_max_; }
  uint64_t maxDefinitionLevel() const override { return d_max_; }

  uint64_t nextRepetitionLevel() override {
    return rlvl_reader_.peek();
  }

  bool eofReached() const override {
    return vals_read_ >= vals_total_;
  }

  void storeMmap(RefPtr<stx::VFSFile> mmap) {
    mmap_ = mmap;
  }

protected:
  uint64_t r_max_;
  uint64_t d_max_;
  stx::util::BinaryMessageReader reader_;
  size_t vals_total_;
  size_t vals_read_;
  uint64_t rlvl_size_;
  uint64_t dlvl_size_;
  uint64_t data_size_;
  stx::util::BitPackDecoder rlvl_reader_;
  stx::util::BitPackDecoder dlvl_reader_;
  void* data_;
  RefPtr<stx::VFSFile> mmap_;
};

} // namespace v1
} // namespace cstable


#endif
