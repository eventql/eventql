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
#include "eventql/io/sstable/sstablereader.h"
#include "eventql/io/sstable/SSTableEditor.h"
#include "eventql/io/sstable/SSTableColumnSchema.h"
#include "eventql/io/sstable/SSTableColumnReader.h"
#include "eventql/io/sstable/SSTableColumnWriter.h"
#include "eventql/io/cstable/cstable_writer.h"
#include "eventql/io/cstable/cstable_reader.h"
#include "eventql/io/cstable/CSTableBuilder.h"

namespace util {
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
} // namespace util

#endif
