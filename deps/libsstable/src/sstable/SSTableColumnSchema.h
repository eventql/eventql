/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_SSTABLE_SSTABLECOLUMNSCHEMA_H
#define _FNORD_SSTABLE_SSTABLECOLUMNSCHEMA_H
#include <stdlib.h>
#include <string>
#include <vector>
#include <memory>
#include <stx/buffer.h>
#include <stx/exception.h>
#include <stx/io/file.h>
#include <stx/io/mmappedfile.h>
#include <sstable/binaryformat.h>
#include <sstable/fileheaderreader.h>
#include <sstable/cursor.h>
#include <sstable/index.h>
#include <sstable/indexprovider.h>

namespace stx {
namespace sstable {
class SSTableEditor;
class SSTableReader;

enum class SSTableColumnType : uint8_t {
  UINT32 = 1,
  UINT64 = 2,
  FLOAT  = 3,
  STRING = 4
};

typedef uint32_t SSTableColumnID;

class SSTableColumnSchema {
public:
  static const uint32_t kSSTableIndexID = 0x34673;

  SSTableColumnSchema();

  void addColumn(
      const String& name,
      SSTableColumnID id,
      SSTableColumnType type);

  SSTableColumnType columnType(SSTableColumnID id) const;
  String columnName(SSTableColumnID id) const;
  SSTableColumnID columnID(const String& column_name) const;
  Set<SSTableColumnID> columnIDs() const;

  void writeIndex(Buffer* buf);
  void writeIndex(SSTableEditor* sstable_writer);

  void loadIndex(const Buffer& buf);
  void loadIndex(SSTableReader* sstable_reader);

protected:
  struct SSTableColumnInfo {
    String name;
    SSTableColumnType type;
  };

  HashMap<SSTableColumnID, SSTableColumnInfo> col_info_;
  HashMap<String, SSTableColumnID> col_ids_;
};

} // namespace sstable
} // namespace stx

#endif
