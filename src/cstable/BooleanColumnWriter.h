/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_CSTABLE_BOOLEANCOLUMNWRITER_H
#define _FNORD_CSTABLE_BOOLEANCOLUMNWRITER_H
#include <stx/stdtypes.h>
#include <stx/util/binarymessagewriter.h>
#include <stx/util/BitPackEncoder.h>
#include <cstable/ColumnWriter.h>

namespace stx {
namespace cstable {

class BooleanColumnWriter : public ColumnWriter {
public:

  BooleanColumnWriter(uint64_t r_max, uint64_t d_max);

  void addDatum(
      uint64_t rep_level,
      uint64_t def_level,
      const void* data,
      size_t size) override;

  void addDatum(uint64_t rep_level, uint64_t def_level, bool value);

  void commit();

  ColumnType type() const override {
    return ColumnType::BOOLEAN;
  }

  msg::FieldType fieldType() const override {
    return msg::FieldType::BOOLEAN;
  }

protected:
  size_t size() const override;
  void write(util::BinaryMessageWriter* writer) override;

  util::BitPackEncoder data_writer_;
};

} // namespace cstable
} // namespace stx

#endif
