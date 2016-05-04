/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_LOGTABLE_TABLE_H
#define _FNORD_LOGTABLE_TABLE_H
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/util/random.h>
#include <eventql/util/io/FileLock.h>
#include <eventql/util/thread/taskscheduler.h>
#include <eventql/util/inspect.h>
#include <eventql/util/protobuf/MessageSchema.h>
#include <eventql/util/protobuf/MessageObject.h>
#include <fnord-afx/ArtifactIndex.h>
#include <fnord-logtable/TableArena.h>
#include <fnord-logtable/TableSnapshot.h>
#include <fnord-logtable/TableChunkSummaryBuilder.h>
#include "eventql/infra/sstable/sstablereader.h"
#include "eventql/infra/sstable/SSTableEditor.h"
#include "eventql/infra/sstable/SSTableColumnSchema.h"
#include "eventql/infra/sstable/SSTableColumnReader.h"
#include "eventql/infra/sstable/SSTableColumnWriter.h"
#include "cstable/CSTableWriter.h"
#include "cstable/CSTableReader.h"
#include "cstable/CSTableBuilder.h"

namespace stx {
namespace logtable {

class TableChunkWriter : public RefCounted {
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
  std::unique_ptr<sstable::SSTableEditor> sstable_;
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
      TableChunkRef* output_chunk,
      RefPtr<TableChunkWriter> writer);

  void merge();

protected:

  void readTable(const String& filename);

  String db_path_;
  String table_name_;
  msg::MessageSchema* schema_;
  RefPtr<TableChunkWriter> writer_;
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
      const String& table_name,
      const String& replica_id,
      const String& db_path,
      const msg::MessageSchema& schema,
      TaskScheduler* scheduler);

  void addSummary(SummaryFactoryFn summary);
  void addRecord(const msg::MessageObject& record);
  void addRecord(const Buffer& record);

  RefPtr<TableSnapshot> getSnapshot();
  const String& name() const;

  size_t commit();
  void merge();
  void gc(size_t keep_generations = 2, size_t max_generations = 10);

  void replicateFrom(const TableGeneration& other_table);

  size_t arenaSize() const;
  ArtifactIndex* artifactIndex();

  void runConsistencyCheck(bool check_checksums = false, bool repair = false);

protected:

  TableWriter(
      const String& table_name,
      const String& replica_id,
      const String& db_path,
      const msg::MessageSchema& schema,
      uint64_t head_sequence,
      RefPtr<TableGeneration> snapshot,
      TaskScheduler* scheduler);

  void gcArenasWithLock();
  size_t commitWithLock();
  void writeTable(RefPtr<TableArena> arena);
  void addChunk(const TableChunkRef* chunk, ArtifactStatus status);
  void writeSnapshot();
  bool merge(size_t min_chunk_size, size_t max_chunk_size);

  String name_;
  String replica_id_;
  String db_path_;
  msg::MessageSchema schema_;
  TaskScheduler* scheduler_;
  ArtifactIndex artifacts_;
  mutable std::mutex mutex_;
  std::mutex merge_mutex_;
  uint64_t seq_;
  List<RefPtr<TableArena>> arenas_;
  Random rnd_;
  RefPtr<TableGeneration> head_;
  TableMergePolicy merge_policy_;
  FileLock lock_;
  List<SummaryFactoryFn> summaries_;
  Duration gc_delay_;
};

} // namespace logtable
} // namespace stx

#endif
