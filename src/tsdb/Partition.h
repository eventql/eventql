/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_TSDB_STREAMCHUNK_H
#define _FNORD_TSDB_STREAMCHUNK_H
#include <stx/stdtypes.h>
#include <stx/option.h>
#include <stx/UnixTime.h>
#include <stx/util/binarymessagereader.h>
#include <stx/util/binarymessagewriter.h>
#include <stx/protobuf/MessageSchema.h>
#include <tsdb/Table.h>
#include <tsdb/RecordIDSet.h>
#include <tsdb/RecordRef.h>
#include <tsdb/TSDBNodeRef.h>
#include <tsdb/PartitionInfo.pb.h>
#include <tsdb/PartitionState.pb.h>
#include <tsdb/PartitionWriter.h>
#include <cstable/CSTableReader.h>

using namespace stx;

namespace tsdb {
class Table;

//struct PartitionState {
//  String stream_key;
//  HashMap<uint64_t, uint64_t> repl_offsets;
//  String cstable_file;
//  SHA1Hash cstable_version;
//  void encode(util::BinaryMessageWriter* writer) const;
//  void decode(util::BinaryMessageReader* reader);
//};

struct PartitionSnapshot : public RefCounted {
  PartitionState state;
  uint64_t nrecs;
};

class Partition : public RefCounted {
public:

  static RefPtr<Partition> create(
      const String& tsdb_namespace,
      RefPtr<Table> table,
      const SHA1Hash& partition_key,
      TSDBNodeRef* node);

  static RefPtr<Partition> reopen(
      const String& tsdb_namespace,
      RefPtr<Table> table,
      const SHA1Hash& partition_key,
      TSDBNodeRef* node);

  //void insertRecord(
  //    const SHA1Hash& record_id,
  //    const Buffer& record);

  //void insertRecords(const Vector<RecordRef>& records);

  RefPtr<PartitionWriter> getWriter();

  PartitionInfo partitionInfo() const;
  Vector<String> listFiles() const;

  Option<RefPtr<VFSFile>> cstableFile() const;


  RefPtr<PartitionSnapshot> getSnapshot() const;
  void commitSnapshot(Function<void (PartitionSnapshot* snap)> fn);

protected:

  Partition(
      RefPtr<PartitionSnapshot> snap,
      RefPtr<Table> table,
      TSDBNodeRef* node);


  void scheduleCompaction();
  uint64_t replicateTo(const String& addr, uint64_t offset);

  void buildCSTable(
    const Vector<String>& input_files,
    const String& output_file);

  RefPtr<PartitionSnapshot> head_;
  mutable std::mutex head_mutex_;

  SHA1Hash key_;
  const RefPtr<Table> table_;
  TSDBNodeRef* node_;
  RefPtr<PartitionWriter> writer_;
};

}
#endif
