/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/util/binarymessagereader.h>
#include <stx/util/BitPackDecoder.h>
#include <cstable/ColumnReader.h>

namespace stx {
namespace cstable {

class DoubleColumnReader : public ColumnReader {
public:

  DoubleColumnReader(
      uint64_t r_max,
      uint64_t d_max,
      void* data,
      size_t size);

  bool next(uint64_t* rep_level, uint64_t* def_level, double* data);

  bool next(
      uint64_t* rep_level,
      uint64_t* def_level,
      void** data,
      size_t* data_len) override;

  msg::FieldType type() const override {
    return msg::FieldType::DOUBLE;
  }

protected:
  util::BinaryMessageReader data_reader_;
  double cur_val_;
};

} // namespace cstable
} // namespace stx

