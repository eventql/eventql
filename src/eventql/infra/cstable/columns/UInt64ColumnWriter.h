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
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/util/binarymessagewriter.h>
#include <eventql/util/util/BitPackEncoder.h>
#include <eventql/infra/cstable/cstable.h>
#include <eventql/infra/cstable/UInt64internal/PageWriter.h>
#include <eventql/infra/cstable/columns/v1/ColumnWriter.h>


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


