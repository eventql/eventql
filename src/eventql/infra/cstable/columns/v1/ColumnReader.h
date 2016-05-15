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
#ifndef _FNORD_CSTABLE_COLUMNREADER_H
#define _FNORD_CSTABLE_COLUMNREADER_H
#include <eventql/infra/cstable/ColumnReader.h>
#include <eventql/util/util/binarymessagereader.h>
#include <eventql/util/util/BitPackDecoder.h>


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

  void storeMmap(RefPtr<util::VFSFile> mmap) {
    mmap_ = mmap;
  }

protected:
  uint64_t r_max_;
  uint64_t d_max_;
  util::BinaryMessageReader reader_;
  size_t vals_total_;
  size_t vals_read_;
  uint64_t rlvl_size_;
  uint64_t dlvl_size_;
  uint64_t data_size_;
  util::BitPackDecoder rlvl_reader_;
  util::BitPackDecoder dlvl_reader_;
  void* data_;
  RefPtr<util::VFSFile> mmap_;
};

} // namespace v1
} // namespace cstable


#endif
