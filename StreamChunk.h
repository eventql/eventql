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
#include <fnord-base/stdtypes.h>
#include <fnord-base/option.h>
#include <fnord-base/datetime.h>
#include <fnord-base/util/binarymessagereader.h>
#include <fnord-base/util/binarymessagewriter.h>
#include <fnord-msg/MessageSchema.h>
#include <fnord-tsdb/StreamProperties.h>
#include <fnord-tsdb/RecordSet.h>
#include <fnord-tsdb/TSDBNodeRef.h>

namespace fnord {
namespace tsdb {

struct StreamChunkState {
  String stream_key;
  RecordSet::RecordSetState record_state;
  HashMap<uint64_t, uint64_t> replicated_offsets;
  void encode(util::BinaryMessageWriter* writer) const;
  void decode(util::BinaryMessageReader* reader);
};

class StreamChunk : public RefCounted {
public:

  static RefPtr<StreamChunk> create(
      const String& streamchunk_key,
      const String& stream_key,
      RefPtr<StreamProperties> config,
      TSDBNodeRef* node);

  static RefPtr<StreamChunk> reopen(
      const String& streamchunk_key,
      const StreamChunkState& state,
      RefPtr<StreamProperties> config,
      TSDBNodeRef* node);

  static String streamChunkKeyFor(
      const String& stream_key,
      DateTime time,
      const StreamProperties& properties);

  static Vector<String> streamChunkKeysFor(
      const String& stream_key,
      DateTime from,
      DateTime until,
      const StreamProperties& properties);

  void insertRecord(
      uint64_t record_id,
      const Buffer& record);

  void compact();
  void replicate();

  Vector<String> listFiles() const;

protected:

  StreamChunk(
      const String& streamchunk_key,
      const String& stream_key,
      RefPtr<StreamProperties> config,
      TSDBNodeRef* node);

  StreamChunk(
      const String& streamchunk_key,
      const StreamChunkState& state,
      RefPtr<StreamProperties> config,
      TSDBNodeRef* node);

  void scheduleCompaction();
  void commitState();
  uint64_t replicateTo(const String& addr, uint64_t offset);

  String key_;
  String stream_key_;
  RecordSet records_;
  RefPtr<StreamProperties> config_;
  TSDBNodeRef* node_;
  std::mutex mutex_;
  std::mutex replication_mutex_;
  bool replication_scheduled_;
  bool compaction_scheduled_;
  DateTime last_compaction_;
  HashMap<uint64_t, uint64_t> replicated_offsets_;
};

}
}
#endif
