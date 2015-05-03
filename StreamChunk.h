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
#include <fnord-msg/MessageSchema.h>
#include <fnord-tsdb/StreamProperties.h>
#include <fnord-tsdb/RecordSet.h>
#include <fnord-tsdb/TSDBNodeRef.h>

namespace fnord {
namespace tsdb {

class StreamChunk : public RefCounted {
public:

  static RefPtr<StreamChunk> create(
      const String& stream_key,
      RefPtr<StreamProperties> config,
      TSDBNodeRef* node);

  static String streamChunkKeyFor(
      const String& stream_key,
      DateTime time,
      const StreamProperties& properties);

  void insertRecord(
      uint64_t record_id,
      const Buffer& record,
      DateTime time);

  void compact();

protected:

  StreamChunk(
      const String& stream_key,
      RefPtr<StreamProperties> config,
      TSDBNodeRef* node);

  void scheduleCompaction();
  void commitState();

  RecordSet records_;
  RefPtr<StreamProperties> config_;
  TSDBNodeRef* node_;
  std::mutex mutex_;
  bool replication_scheduled_;
  bool compaction_scheduled_;
  DateTime last_compaction_;
};

}
}
#endif
