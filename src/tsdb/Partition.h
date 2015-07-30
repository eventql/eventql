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

  void insertRecord(
      const SHA1Hash& record_id,
      const Buffer& record);

  void insertRecords(const Vector<RecordRef>& records);

  PartitionInfo partitionInfo() const;
  Vector<String> listFiles() const;

  Option<RefPtr<VFSFile>> cstableFile() const;

  void compact();
  void replicate();

  void commit();

protected:

  Partition(
      const PartitionState& state,
      RefPtr<Table> table,
      TSDBNodeRef* node);

  void scheduleCompaction();
  
  uint64_t replicateTo(const String& addr, uint64_t offset);

  void buildCSTable(
    const Vector<String>& input_files,
    const String& output_file);

  const PartitionState& state_;
  SHA1Hash key_;
  const RefPtr<Table> table_;
  TSDBNodeRef* node_;
  RecordIDSet recids_;
  mutable std::mutex mutex_;
  std::mutex replication_mutex_;
  //UnixTime last_compaction_;
  //HashMap<uint64_t, uint64_t> repl_offsets_;
  //String cstable_file_;
  //SHA1Hash cstable_version_;
};

}
#endif
