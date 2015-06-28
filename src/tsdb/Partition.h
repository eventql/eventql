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
#include <fnord/stdtypes.h>
#include <fnord/option.h>
#include <fnord/datetime.h>
#include <fnord/util/binarymessagereader.h>
#include <fnord/util/binarymessagewriter.h>
#include <fnord/protobuf/MessageSchema.h>
#include <tsdb/StreamConfig.pb.h>
#include <tsdb/RecordSet.h>
#include <tsdb/TSDBNodeRef.h>
#include <tsdb/PartitionInfo.pb.h>

using namespace fnord;

namespace tsdb {

struct PartitionState {
  String stream_key;
  RecordSet::RecordSetState record_state;
  HashMap<uint64_t, uint64_t> repl_offsets;
  String cstable_file;
  SHA1Hash cstable_version;
  void encode(util::BinaryMessageWriter* writer) const;
  void decode(util::BinaryMessageReader* reader);
};

class Partition : public RefCounted {
public:

  static RefPtr<Partition> create(
      const SHA1Hash& partition_key,
      const String& stream_key,
      const String& db_key,
      StreamConfig* config,
      TSDBNodeRef* node);

  static RefPtr<Partition> reopen(
      const SHA1Hash& partition_key,
      const PartitionState& state,
      const String& db_key,
      StreamConfig* config,
      TSDBNodeRef* node);

  void insertRecord(
      const SHA1Hash& record_id,
      const Buffer& record);

  void insertRecords(const Vector<RecordRef>& records);

  PartitionInfo partitionInfo() const;
  Vector<String> listFiles() const;

  void compact();
  void replicate();

protected:

  Partition(
      const SHA1Hash& partition_key,
      const String& stream_key,
      const String& db_key,
      StreamConfig* config,
      TSDBNodeRef* node);

  Partition(
      const SHA1Hash& partition_key,
      const PartitionState& state,
      const String& db_key,
      StreamConfig* config,
      TSDBNodeRef* node);

  void scheduleCompaction();
  void commitState();
  uint64_t replicateTo(const String& addr, uint64_t offset);

  SHA1Hash key_;
  String stream_key_;
  String db_key_;
  RecordSet records_;
  StreamConfig* config_;
  TSDBNodeRef* node_;
  std::mutex mutex_;
  std::mutex replication_mutex_;
  DateTime last_compaction_;
  HashMap<uint64_t, uint64_t> repl_offsets_;
  String cstable_file_;
  SHA1Hash cstable_version_;
};

}
#endif
