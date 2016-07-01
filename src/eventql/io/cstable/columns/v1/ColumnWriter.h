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
#ifndef _FNORD_CSTABLE_COLUMNWRITER_H
#define _FNORD_CSTABLE_COLUMNWRITER_H
#include <eventql/util/stdtypes.h>
#include <eventql/util/exception.h>
#include <eventql/util/autoref.h>
#include <eventql/util/util/binarymessagewriter.h>
#include <eventql/util/util/BitPackEncoder.h>
#include <eventql/util/protobuf/MessageObject.h>
#include <eventql/io/cstable/cstable.h>
#include <eventql/io/cstable/ColumnWriter.h>


namespace cstable {
namespace v1 {

class ColumnWriter : public cstable::ColumnWriter {
public:

  ColumnWriter(size_t r_max, size_t d_max);

  void writeNull(uint64_t rlvl, uint64_t dlvl) override;

  void writeBoolean(
      uint64_t rlvl,
      uint64_t dlvl,
      bool value) override;

  void writeUnsignedInt(
      uint64_t rlvl,
      uint64_t dlvl,
      uint64_t value) override;

  void writeSignedInt(
      uint64_t rlvl,
      uint64_t dlvl,
      int64_t value) override;

  void writeFloat(
      uint64_t rlvl,
      uint64_t dlvl,
      double value) override;

  void writeString(
      uint64_t rlvl,
      uint64_t dlvl,
      const char* data,
      size_t size) override;

  void writeDateTime(
      uint64_t rlvl,
      uint64_t dlvl,
      UnixTime value) override;

  void write(void* buf, size_t buf_len);
  virtual void commit();

  virtual size_t bodySize() const;

  /**
   * Deprecated, do not use
   */
  virtual void addDatum(
      uint64_t rep_level,
      uint64_t def_level,
      const void* data,
      size_t size) = 0;

  void flush() override {}

protected:
  virtual size_t size() const = 0;
  virtual void write(util::BinaryMessageWriter* writer) = 0;

  size_t r_max_;
  size_t d_max_;
  util::BitPackEncoder rlvl_writer_;
  util::BitPackEncoder dlvl_writer_;
  size_t num_vals_;
};

} // namespace v1
} // namespace cstable


#endif
