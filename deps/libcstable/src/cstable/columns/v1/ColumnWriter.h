/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_CSTABLE_COLUMNWRITER_H
#define _FNORD_CSTABLE_COLUMNWRITER_H
#include <stx/stdtypes.h>
#include <stx/exception.h>
#include <stx/autoref.h>
#include <stx/util/binarymessagewriter.h>
#include <stx/util/BitPackEncoder.h>
#include <stx/protobuf/MessageObject.h>
#include <cstable/cstable.h>
#include <cstable/ColumnWriter.h>


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

protected:
  virtual size_t size() const = 0;
  virtual void write(stx::util::BinaryMessageWriter* writer) = 0;

  size_t r_max_;
  size_t d_max_;
  stx::util::BitPackEncoder rlvl_writer_;
  stx::util::BitPackEncoder dlvl_writer_;
  size_t num_vals_;
};

} // namespace v1
} // namespace cstable


#endif
