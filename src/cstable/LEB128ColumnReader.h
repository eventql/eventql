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
#include <cstable/ColumnReader.h>

namespace stx {
namespace cstable {

class LEB128ColumnReader : public ColumnReader {
public:

  LEB128ColumnReader(
      uint64_t r_max,
      uint64_t d_max,
      void* data,
      size_t size);

  bool next(uint64_t* rep_level, uint64_t* def_level, uint64_t* data);

  bool next(
      uint64_t* rep_level,
      uint64_t* def_level,
      void** data,
      size_t* data_len) override;

  msg::FieldType type() const override {
    return msg::FieldType::UINT64;
  }

protected:
  util::BinaryMessageReader data_reader_;
  uint64_t cur_val_;
};

} // namespace cstable
} // namespace stx

#endif
