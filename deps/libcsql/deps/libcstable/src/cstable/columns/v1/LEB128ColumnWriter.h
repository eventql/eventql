/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_CSTABLE_LEB1238COLUMNWRITER_H
#define _FNORD_CSTABLE_LEB1238COLUMNWRITER_H
#include <stx/stdtypes.h>
#include <stx/util/binarymessagewriter.h>
#include <stx/util/BitPackEncoder.h>
#include <cstable/cstable.h>
#include <cstable/columns/v1/ColumnWriter.h>


namespace cstable {
namespace v1 {

class LEB128ColumnWriter : public ColumnWriter {
public:

  LEB128ColumnWriter(
      uint64_t r_max,
      uint64_t d_max,
      ColumnType type);

  void addDatum(
      uint64_t rep_level,
      uint64_t def_level,
      const void* data,
      size_t size) override;

  void addDatum(uint64_t rep_level, uint64_t def_level, uint64_t value);

  ColumnEncoding encoding() const override {
    return ColumnEncoding::UINT64_LEB128;
  }

  ColumnType type() const override {
    return type_;
  }

protected:
  size_t size() const override;
  void write(stx::util::BinaryMessageWriter* writer) override;

  ColumnType type_;
  stx::util::BinaryMessageWriter data_writer_;
};

} // namespace v1
} // namespace cstable


#endif
