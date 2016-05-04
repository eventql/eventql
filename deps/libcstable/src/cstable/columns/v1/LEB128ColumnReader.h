/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_CSTABLE_LEB128COLUMNREADER_H
#define _FNORD_CSTABLE_LEB128COLUMNREADER_H
#include <stx/stdtypes.h>
#include <stx/util/binarymessagereader.h>
#include <stx/util/BitPackDecoder.h>
#include <cstable/columns/v1/ColumnReader.h>


namespace cstable {
namespace v1 {

class LEB128ColumnReader : public ColumnReader {
public:

  LEB128ColumnReader(
      uint64_t r_max,
      uint64_t d_max,
      void* data,
      size_t size);

  bool readBoolean(
      uint64_t* rlvl,
      uint64_t* dlvl,
      bool* value) override;

  bool readUnsignedInt(
      uint64_t* rlvl,
      uint64_t* dlvl,
      uint64_t* value) override;

  bool readSignedInt(
      uint64_t* rlvl,
      uint64_t* dlvl,
      int64_t* value) override;

  bool readFloat(
      uint64_t* rlvl,
      uint64_t* dlvl,
      double* value) override;

  bool readString(
      uint64_t* rlvl,
      uint64_t* dlvl,
      String* value) override;

  void skipValue() override;
  void copyValue(ColumnWriter* writer) override;

  ColumnType type() const override {
    return ColumnType::UNSIGNED_INT;
  }

  ColumnEncoding encoding() const override {
    return ColumnEncoding::UINT64_LEB128;
  }

protected:
  stx::util::BinaryMessageReader data_reader_;
  uint64_t cur_val_;
};

} // namespace v1
} // namespace cstable


#endif
