/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_EVENTDB_TABLE_H
#define _FNORD_EVENTDB_TABLE_H
#include <fnord-base/stdtypes.h>
#include <fnord-base/autoref.h>
#include <fnord-base/random.h>
#include <fnord-msg/MessageSchema.h>
#include <fnord-msg/MessageObject.h>
#include <fnord-eventdb/TableArena.h>
#include "fnord-sstable/sstablereader.h"
#include "fnord-sstable/sstablewriter.h"
#include "fnord-sstable/SSTableColumnSchema.h"
#include "fnord-sstable/SSTableColumnReader.h"
#include "fnord-sstable/SSTableColumnWriter.h"
#include "fnord-cstable/CSTableWriter.h"
#include "fnord-cstable/CSTableReader.h"
#include "fnord-cstable/CSTableBuilder.h"

namespace fnord {
namespace eventdb {

struct TableChunkRef {
  String replica_id;
  String chunk_id;
  uint64_t start_sequence;
  uint64_t num_records;
  uint64_t sstable_checksum;
  uint64_t cstable_checksum;
  uint64_t index_checksum;
};

struct TableGeneration : public RefCounted {
  String table_name;
  uint64_t generation;
  Vector<TableChunkRef> chunks;

  RefPtr<TableGeneration> clone() const;
  void encode(Buffer* buf);
  void decode(const Buffer& buf);
};

struct TableSnapshot : public RefCounted {
  TableSnapshot(
      RefPtr<TableGeneration> _head,
      List<RefPtr<TableArena>> _arenas);

  RefPtr<TableGeneration> head;
  List<RefPtr<TableArena>> arenas;
};

class TableChunkWriter {
public:

  TableChunkWriter(
      const String& db_path,
      const String& table_name,
      msg::MessageSchema* schema,
      TableChunkRef* chunk);

  void addRecord(const msg::MessageObject& record);
  void commit();

protected:
  msg::MessageSchema* schema_;
  TableChunkRef* chunk_;
  size_t seq_;
  String chunk_name_;
  String chunk_filename_;
  std::unique_ptr<sstable::SSTableWriter> sstable_;
  std::unique_ptr<cstable::CSTableBuilder> cstable_;
};

class Table : public RefCounted {
public:

  static const size_t kDefaultMaxChunkSize = 1024 * 1024 * 25; // 25MB

  static RefPtr<Table> open(
      const String& table_name,
      const String& replica_id,
      const String& db_path,
      const msg::MessageSchema& schema);

  void addRecords(const Buffer& records);
  void addRecord(const msg::MessageObject& record);

  const String& name() const;

  size_t commit();
  void merge();

  RefPtr<TableSnapshot> getSnapshot();

protected:

  Table(
      const String& table_name,
      const String& replica_id,
      const String& db_path,
      const msg::MessageSchema& schema,
      uint64_t head_sequence,
      RefPtr<TableGeneration> snapshot);

  void writeTable(RefPtr<TableArena> arena);
  void addChunk(TableChunkRef chunk);
  void writeSnapshot();

  String name_;
  String replica_id_;
  String db_path_;
  msg::MessageSchema schema_;
  std::mutex mutex_;
  std::mutex merge_mutex_;
  uint64_t seq_;
  List<RefPtr<TableArena>> arenas_;
  Random rnd_;
  RefPtr<TableGeneration> head_;
  size_t max_chunk_size_;
};

} // namespace eventdb
} // namespace fnord

#endif
