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
#include <fnord-tsdb/StreamConfig.pb.h>
#include <fnord-tsdb/RecordSet.h>
#include <fnord-tsdb/TSDBNodeRef.h>
#include <fnord-tsdb/PartitionInfo.pb.h>

using namespace fnord;

namespace tsdb {

struct StreamChunkState {
  String stream_key;
  RecordSet::RecordSetState record_state;
  HashMap<uint64_t, uint64_t> repl_offsets;
  void encode(util::BinaryMessageWriter* writer) const;
  void decode(util::BinaryMessageReader* reader);
};

class StreamChunk : public RefCounted {
public:

  static RefPtr<StreamChunk> create(
      const SHA1Hash& partition_key,
      const String& stream_key,
      StreamConfig* config,
      TSDBNodeRef* node);

  static RefPtr<StreamChunk> reopen(
      const SHA1Hash& partition_key,
      const StreamChunkState& state,
      StreamConfig* config,
      TSDBNodeRef* node);

  //static String streamChunkKeyFor(
  //    const String& stream_key,
  //    DateTime time,
  //    Duration partition_size);

  //static String streamChunkKeyFor(
  //    const String& stream_key,
  //    DateTime time,
  //    const StreamConfig& properties);

  //static Vector<String> streamChunkKeysFor(
  //    const String& stream_key,
  //    DateTime from,
  //    DateTime until,
  //    const StreamConfig& properties);

  void insertRecord(
      const SHA1Hash& record_id,
      const Buffer& record);

  void insertRecords(const Vector<RecordRef>& records);

  PartitionInfo partitionInfo() const;
  Vector<String> listFiles() const;

  void compact();
  void replicate();

protected:

  StreamChunk(
      const SHA1Hash& partition_key,
      const String& stream_key,
      StreamConfig* config,
      TSDBNodeRef* node);

  StreamChunk(
      const SHA1Hash& partition_key,
      const StreamChunkState& state,
      StreamConfig* config,
      TSDBNodeRef* node);

  void scheduleCompaction();
  void commitState();
  uint64_t replicateTo(const String& addr, uint64_t offset);

  SHA1Hash key_;
  String stream_key_;
  RecordSet records_;
  StreamConfig* config_;
  TSDBNodeRef* node_;
  std::mutex mutex_;
  std::mutex replication_mutex_;
  DateTime last_compaction_;
  HashMap<uint64_t, uint64_t> repl_offsets_;
};

}
#endif
