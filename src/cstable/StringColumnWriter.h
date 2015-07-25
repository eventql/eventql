/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_CSTABLE_STRINGCOLUMNWRITER_H
#define _FNORD_CSTABLE_STRINGCOLUMNWRITER_H
#include <stx/stdtypes.h>
#include <stx/util/binarymessagewriter.h>
#include <stx/util/BitPackEncoder.h>
#include <cstable/BinaryFormat.h>
#include <cstable/ColumnWriter.h>

namespace stx {
namespace cstable {

class StringColumnWriter : public ColumnWriter {
public:

  StringColumnWriter(
      uint64_t r_max,
      uint64_t d_max,
      size_t max_strlen = -1);

  void addDatum(
      uint64_t rep_level,
      uint64_t def_level,
      const void* data,
      size_t size) override;

  void addDatum(uint64_t rep_level, uint64_t def_level, const String& value);

  ColumnType type() const override {
    return ColumnType::STRING_PLAIN;
  }

  msg::FieldType fieldType() const override {
    return msg::FieldType::STRING;
  }

protected:
  size_t size() const override;
  void write(util::BinaryMessageWriter* writer) override;

  util::BinaryMessageWriter data_writer_;
};

} // namespace cstable
} // namespace stx

#endif
