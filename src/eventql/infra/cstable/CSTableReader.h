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
#include <eventql/util/stdtypes.h>
#include <eventql/util/io/file.h>
#include <eventql/infra/cstable/ColumnReader.h>
#include <eventql/infra/cstable/cstable.h>
#include <eventql/infra/cstable/io/PageManager.h>

namespace cstable {

class CSTableReader : public stx::RefCounted {
public:

  static RefPtr<CSTableReader> openFile(const String& filename);

  bool hasColumn(const String& column_name) const ;

  RefPtr<ColumnReader> getColumnReader(const String& column_name);
  ColumnType getColumnType(const String& column_name);
  ColumnEncoding getColumnEncoding(const String& column_name);

  const Vector<ColumnConfig>& columns() const;

  size_t numRecords() const;

protected:

  CSTableReader(
      BinaryFormatVersion version,
      Vector<ColumnConfig> columns,
      Vector<RefPtr<ColumnReader>> column_readers,
      uint64_t num_rows);

  BinaryFormatVersion version_;
  Vector<ColumnConfig> columns_;
  HashMap<uint32_t, RefPtr<ColumnReader>> column_readers_by_id_;
  HashMap<String, RefPtr<ColumnReader>> column_readers_by_name_;
  uint64_t num_rows_;
};

} // namespace cstable


