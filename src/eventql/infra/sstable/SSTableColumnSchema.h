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
#ifndef _FNORD_SSTABLE_SSTABLECOLUMNSCHEMA_H
#define _FNORD_SSTABLE_SSTABLECOLUMNSCHEMA_H
#include <stdlib.h>
#include <string>
#include <vector>
#include <memory>
#include <eventql/util/buffer.h>
#include <eventql/util/exception.h>
#include <eventql/util/io/file.h>
#include <eventql/util/io/mmappedfile.h>
#include <eventql/infra/sstable/binaryformat.h>
#include <eventql/infra/sstable/fileheaderreader.h>
#include <eventql/infra/sstable/cursor.h>
#include <eventql/infra/sstable/index.h>
#include <eventql/infra/sstable/indexprovider.h>


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


#endif
