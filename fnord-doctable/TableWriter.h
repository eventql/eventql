/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_DOCTABLE_TABLE_H
#define _FNORD_DOCTABLE_TABLE_H
#include <fnord-base/stdtypes.h>
#include <fnord-base/autoref.h>
#include <fnord-base/random.h>
#include <fnord-base/io/FileLock.h>
#include <fnord-base/thread/taskscheduler.h>
#include <fnord-msg/MessageSchema.h>
#include <fnord-msg/MessageObject.h>
#include <fnord-logtable/ArtifactIndex.h>
#include <fnord-doctable/TableArena.h>
#include <fnord-doctable/TableSnapshot.h>
#include "fnord-sstable/sstablereader.h"
#include "fnord-sstable/sstablewriter.h"
#include "fnord-sstable/SSTableColumnSchema.h"
#include "fnord-sstable/SSTableColumnReader.h"
#include "fnord-sstable/SSTableColumnWriter.h"

namespace fnord {
namespace doctable {

class TableWriter : public RefCounted {
public:

  static RefPtr<TableWriter> open(
      const String& table_name,
      const String& db_path,
      const msg::MessageSchema& schema);

  void updateDocument(const String& key, const msg::MessageObject& doc);

  RefPtr<TableSnapshot> getSnapshot();
  const String& name() const;

  size_t commit();
  //void merge();
  //void gc(size_t keep_generations = 2, size_t max_generations = 10);

  size_t arenaSize() const;
  ArtifactIndex* artifactIndex();

  void runConsistencyCheck(bool check_checksums = false, bool repair = false);

protected:

  TableWriter(
      const String& table_name,
      const String& db_path,
      const msg::MessageSchema& schema,
      RefPtr<TableSnapshot> snapshot);

  void gcArenasWithLock();
  size_t commitWithLock();
  void writeTable(RefPtr<TableArena> arena);
  void addChunk(const TableSegmentRef* chunk, ArtifactStatus status);
  void writeSnapshot();
  bool merge(size_t min_chunk_size, size_t max_chunk_size);

  String name_;
  String db_path_;
  msg::MessageSchema schema_;
  ArtifactIndex artifacts_;
  mutable std::mutex mutex_;
  std::mutex merge_mutex_;
  RefPtr<TableArena> arena_;
  Random rnd_;
  RefPtr<TableSnapshot> snap_;
  FileLock lock_;
};

} // namespace doctable
} // namespace fnord

#endif
