/**
 * This file is part of the "libcstable" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * libcstable is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/util/binarymessagewriter.h>
#include <stx/util/BitPackEncoder.h>
#include <cstable/cstable.h>
#include <cstable/UInt64internal/PageWriter.h>
#include <cstable/columns/v1/ColumnWriter.h>


namespace cstable {

class UInt64ColumnWriter : public ColumnWriter {
public:

  UInt64ColumnWriter(
      RefPtr<PageManager> page_mgr,
      RefPtr<Buffer> meta_buf,
      RefPtr<Buffer> rlevel_meta_buf,
      RefPtr<Buffer> dlevel_meta_buf,
      uint64_t r_max,
      uint64_t d_max,
      uint32_t max_value = 0xffffffff);

  void addNull(uint64_t rep_level, uint64_t def_level) override;

  void addDatum(
      uint64_t rep_level,
      uint64_t def_level,
      const void* data,
      size_t size) override;

  void addDatum(uint64_t rep_level, uint64_t def_level, uint32_t value);
  void commit();

  ColumnEncoding encoding() const override {
    return ColumnEncoding::UINT64_PLAIN;
  }

  ColumnType type() const override {
    return ColumnType::UNSIGNED_INT;
  }

protected:
  uint32_t max_value_;
  UInt64PageWriter data_writer_;
  UInt64PageWriter rlevel_writer_;
  UInt64PageWriter dlevel_writer_;
};

} // namespace cstable


