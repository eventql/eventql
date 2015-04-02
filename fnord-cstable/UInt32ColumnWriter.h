/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_CSTABLE_UINT16COLUMNWRITER_H
#define _FNORD_CSTABLE_UINT16COLUMNWRITER_H
#include <fnord-base/stdtypes.h>
#include <fnord-base/util/binarymessagewriter.h>
#include <fnord-base/util/BitPackEncoder.h>
#include <fnord-cstable/BinaryFormat.h>
#include <fnord-cstable/ColumnWriter.h>

namespace fnord {
namespace cstable {

class UInt32ColumnWriter : public ColumnWriter {
public:

  UInt32ColumnWriter(
      uint64_t r_max,
      uint64_t d_max,
      uint32_t max_value = 0xffffffff);

  void addDatum(uint64_t rep_level, uint64_t def_level, uint16_t value);
  void addNull(uint64_t rep_level, uint64_t def_level);
  void commit();

  ColumnType type() const override {
    return ColumnType::UINT32_BITPACKED;
  }

protected:
  size_t size() const override;
  void write(util::BinaryMessageWriter* writer) override;

  uint32_t max_value_;
  util::BitPackEncoder data_writer_;
};

} // namespace cstable
} // namespace fnord

#endif
