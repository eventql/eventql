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
#include <cstable/ColumnReader.h>
#include <cstable/io/PageReader.h>
#include <cstable/io/PageManager.h>
#include <cstable/io/PageIndex.h>


namespace cstable {

class UnsignedIntColumnReader : public ColumnReader {
public:

  UnsignedIntColumnReader(
      ColumnConfig config,
      ScopedPtr<UnsignedIntPageReader> rlevel_reader,
      ScopedPtr<UnsignedIntPageReader> dlevel_reader,
      RefPtr<PageManager> page_mgr,
      PageIndexReader* page_idx);

  bool readBoolean(
      uint64_t* rlvl,
      uint64_t* dlvl,
      bool* value) override;

  bool readUnsignedInt(
      uint64_t* rlvl,
      uint64_t* dlvl,
      uint64_t* value) override;

  bool readSignedInt(
      uint64_t* rlvl,
      uint64_t* dlvl,
      int64_t* value) override;

  bool readFloat(
      uint64_t* rlvl,
      uint64_t* dlvl,
      double* value) override;

  bool readString(
      uint64_t* rlvl,
      uint64_t* dlvl,
      String* value) override;

  void skipValue() override;
  void copyValue(ColumnWriter* writer) override;

  ColumnType type() const override;
  ColumnEncoding encoding() const override;

  uint64_t maxRepetitionLevel() const override;
  uint64_t maxDefinitionLevel() const override;

  uint64_t nextRepetitionLevel() override;

  bool eofReached() const override;

protected:
  ColumnConfig config_;
  ScopedPtr<UnsignedIntPageReader> rlevel_reader_;
  ScopedPtr<UnsignedIntPageReader> dlevel_reader_;
  ScopedPtr<UnsignedIntPageReader> data_reader_;
};

} // namespace cstable


