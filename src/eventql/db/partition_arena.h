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
#include "eventql/eventql.h"
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/util/status.h>
#include <eventql/util/protobuf/MessageObject.h>
#include <eventql/io/cstable/RecordShredder.h>
#include <eventql/io/cstable/cstable_writer.h>
#include <eventql/io/cstable/cstable_file.h>
#include <eventql/db/RecordRef.h>
#include <eventql/db/shredded_record.h>

namespace eventql {

class PartitionArena : public RefCounted {
public:

  struct SkiplistReader {
    bool readNext();
    size_t size() const;
    Vector<bool> skiplist;
    size_t position;
  };

  PartitionArena(const msg::MessageSchema& schema);

  bool insertRecord(const RecordRef& record);

  bool insertRecord(
      const SHA1Hash& record_id,
      uint64_t record_version,
      const msg::MessageObject& record,
      bool is_update);

  Set<SHA1Hash> insertRecords(
      const ShreddedRecordList& records,
      Vector<bool> skip_flags,
      const Vector<bool>& update_flags);

  uint64_t fetchRecordVersion(const SHA1Hash& record_id);

  size_t size() const;

  Status writeToDisk(
      const String& filename,
      uint64_t sequence);

  cstable::CSTableFile* getCSTableFile() const;
  SkiplistReader getSkiplistReader() const;

protected:
  struct RecordVersion {
    uint64_t version;
    uint64_t position;
  };

  void open();

  msg::MessageSchema schema_;
  mutable std::mutex mutex_;
  HashMap<SHA1Hash, RecordVersion> record_versions_;
  Vector<bool> skiplist_;
  size_t num_records_;
  OrderedMap<SHA1Hash, uint64_t> vmap_;
  cstable::TableSchema cstable_schema_;
  cstable::TableSchema cstable_schema_ext_;
  ScopedPtr<cstable::CSTableFile> cstable_file_;
  RefPtr<cstable::CSTableWriter> cstable_writer_;
  RefPtr<cstable::ColumnWriter> is_update_col_;
  RefPtr<cstable::ColumnWriter> id_col_;
  RefPtr<cstable::ColumnWriter> version_col_;
  bool opened_;
};

} // namespace eventql

