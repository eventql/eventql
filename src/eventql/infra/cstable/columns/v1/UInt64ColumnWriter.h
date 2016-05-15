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
#ifndef _FNORD_CSTABLE_INT64COLUMNWRITER_H
#define _FNORD_CSTABLE_INT64COLUMNWRITER_H
#include <eventql/util/stdtypes.h>
#include <eventql/util/util/binarymessagewriter.h>
#include <eventql/util/util/BitPackEncoder.h>
#include <eventql/infra/cstable/cstable.h>
#include <eventql/infra/cstable/columns/v1/ColumnWriter.h>


namespace cstable {
namespace v1 {

class UInt64ColumnWriter : public ColumnWriter {
public:

  UInt64ColumnWriter(
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
    return ColumnEncoding::UINT64_PLAIN;
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
