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
#include <stx/exception.h>
#include <stx/autoref.h>
#include <stx/protobuf/MessageObject.h>
#include <cstable/cstable.h>
#include <cstable/io/PageManager.h>
#include <cstable/io/PageIndex.h>
#include <cstable/columns/UInt64PageWriter.h>

namespace cstable {

class ColumnWriter : public stx::RefCounted {
public:

  ColumnWriter(size_t r_max, size_t d_max);

  virtual void writeNull(uint64_t rlvl, uint64_t dlvl) = 0;

  virtual void writeBoolean(
      uint64_t rlvl,
      uint64_t dlvl,
      bool value) = 0;

  virtual void writeUnsignedInt(
      uint64_t rlvl,
      uint64_t dlvl,
      uint64_t value) = 0;

  virtual void writeSignedInt(
      uint64_t rlvl,
      uint64_t dlvl,
      int64_t value) = 0;

  virtual void writeFloat(
      uint64_t rlvl,
      uint64_t dlvl,
      double value) = 0;

  void writeString(
      uint64_t rlvl,
      uint64_t dlvl,
      const String& value);

  virtual void writeString(
      uint64_t rlvl,
      uint64_t dlvl,
      const char* data,
      size_t size) = 0;

  virtual void writeDateTime(
      uint64_t rlvl,
      uint64_t dlvl,
      UnixTime value);

  virtual ColumnType type() const = 0;
  virtual ColumnEncoding encoding() const = 0;

  size_t maxRepetitionLevel() const;
  size_t maxDefinitionLevel() const;

protected:
  size_t r_max_;
  size_t d_max_;
};

class DefaultColumnWriter : public ColumnWriter {
public:

  DefaultColumnWriter(
      ColumnConfig config,
      RefPtr<PageManager> page_mgr,
      RefPtr<PageIndex> page_idx);

  void writeNull(uint64_t rlvl, uint64_t dlvl) override;

  ColumnEncoding encoding() const override {
    return ColumnEncoding::UINT32_BITPACKED;
  }

  ColumnType type() const override {
    return ColumnType::UNSIGNED_INT;
  }

protected:
  ColumnConfig config_;
  ScopedPtr<UnsignedIntPageWriter> rlevel_writer_;
  ScopedPtr<UnsignedIntPageWriter> dlevel_writer_;
};

} // namespace cstable


