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
#include <eventql/util/io/file.h>
#include <eventql/io/cstable/ColumnReader.h>
#include <eventql/io/cstable/cstable.h>
#include <eventql/io/cstable/page_manager.h>

namespace cstable {

class CSTableReader : public RefCounted {
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


