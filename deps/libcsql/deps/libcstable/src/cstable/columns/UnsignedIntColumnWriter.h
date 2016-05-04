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
#include <cstable/ColumnWriter.h>


namespace cstable {

class UnsignedIntColumnWriter : public DefaultColumnWriter {
public:

  UnsignedIntColumnWriter(
      ColumnConfig config,
      RefPtr<PageManager> page_mgr,
      RefPtr<PageIndex> page_idx);

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

protected:
  ScopedPtr<UnsignedIntPageWriter> data_writer_;
};

} // namespace cstable


