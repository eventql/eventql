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
#include <fnord-base/io/FileLock.h>
#include <fnord-msg/MessageSchema.h>
#include <fnord-msg/MessageObject.h>
#include <fnord-eventdb/ArtifactIndex.h>
#include <fnord-eventdb/TableArena.h>
#include <fnord-eventdb/TableSnapshot.h>
#include <fnord-eventdb/TableChunkSummaryBuilder.h>
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

class TableChunkWriter {
public:

  TableChunkWriter(
      const String& db_path,
      const String& table_name,
      msg::MessageSchema* schema,
      TableChunkRef* chunk);

  void addRecord(const msg::MessageObject& record);
  void addSummary(RefPtr<TableChunkSummaryBuilder> summary);
  void commit();

protected:
  msg::MessageSchema* schema_;
  TableChunkRef* chunk_;
  size_t seq_;
  String chunk_name_;
  String chunk_filename_;
  std::unique_ptr<sstable::SSTableWriter> sstable_;
  std::unique_ptr<cstable::CSTableBuilder> cstable_;
  List<RefPtr<TableChunkSummaryBuilder>> summaries_;
};

class TableChunkMerge {
public:

  TableChunkMerge(
      const String& db_path,
      const String& table_name,
      msg::MessageSchema* schema,
      const Vector<TableChunkRef> input_chunks,
      TableChunkRef* output_chunk);

  void merge();

protected:

  void readTable(const String& filename);

  String db_path_;
  String table_name_;
  msg::MessageSchema* schema_;
  TableChunkWriter writer_;
  Vector<TableChunkRef> input_chunks_;
};

class TableMergePolicy {
public:

  TableMergePolicy();

  bool findNextMerge(
      RefPtr<TableSnapshot> snapshot,
      const String& db_path,
      const String& table_name,
      const String& replica_id,
      Vector<TableChunkRef>* input_chunks,
      TableChunkRef* output_chunk);

protected:

  bool tryFoldIntoMerge(
      const String& db_path,
      const String& table_name,
      const String& replica_id,
      size_t min_merged_size,
      size_t max_merged_size,
      const Vector<TableChunkRef>& chunks,
      size_t idx,
      Vector<TableChunkRef>* input_chunks,
      TableChunkRef* output_chunk);

  Vector<Pair<uint64_t, uint64_t>> steps_;
  Random rnd_;
};

class TableWriter : public RefCounted {
public:
  typedef Function<RefPtr<TableChunkSummaryBuilder> ()> SummaryFactoryFn;

  static RefPtr<TableWriter> open(
      ArtifactIndex* artifacts,
      const String& table_name,
      const String& replica_id,
      const String& db_path,
      const msg::MessageSchema& schema);

  void addSummary(SummaryFactoryFn summary);
  void addRecords(const Buffer& records);
  void addRecord(const msg::MessageObject& record);

  RefPtr<TableSnapshot> getSnapshot();
  const String& name() const;

  size_t commit();
  void merge();
  void gc(size_t keep_generations = 2, size_t keep_arenas = 0);

  void replicateFrom(const TableGeneration& other_table);

protected:

  TableWriter(
      ArtifactIndex* artifacts,
      const String& table_name,
      const String& replica_id,
      const String& db_path,
      const msg::MessageSchema& schema,
      uint64_t head_sequence,
      RefPtr<TableGeneration> snapshot);

  size_t commitWithLock();
  void writeTable(RefPtr<TableArena> arena);
  void addChunk(const TableChunkRef* chunk, ArtifactStatus status);
  void writeSnapshot();
  bool merge(size_t min_chunk_size, size_t max_chunk_size);

  ArtifactIndex* artifacts_;
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
  TableMergePolicy merge_policy_;
  FileLock lock_;
  List<SummaryFactoryFn> summaries_;
};

} // namespace eventdb
} // namespace fnord

#endif
