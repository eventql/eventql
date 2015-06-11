/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_TSDB_TSDBNODE_H
#define _FNORD_TSDB_TSDBNODE_H
#include <fnord-base/stdtypes.h>
#include <fnord-base/random.h>
#include <fnord-base/option.h>
#include <fnord-base/thread/queue.h>
#include <fnord-mdb/MDB.h>
#include <fnord-tsdb/StreamConfig.pb.h>
#include <fnord-tsdb/StreamChunk.h>
#include <fnord-tsdb/TSDBNodeRef.h>
#include <fnord-tsdb/CompactionWorker.h>
#include <fnord-tsdb/ReplicationWorker.h>

using namespace fnord;

namespace tsdb {

class TSDBNode {
public:

  TSDBNode(
      const String& db_path,
      RefPtr<dht::ReplicationScheme> replication_scheme,
      http::HTTPConnectionPool* http);

  void configurePrefix(
      const String& stream_namespace,
      StreamConfig config);

  StreamConfig* configFor(
      const String& stream_namespace,
      const String& stream_key) const;

  StreamChunk* lookupPartition(
      const String& stream_namespace,
      const String& stream_key,
      const SHA1Hash& partition_key);

  StreamChunk* lookupOrCreatePartition(
      const String& stream_namespace,
      const String& stream_key,
      const SHA1Hash& partition_key);

  void start(
      size_t num_comaction_threads = 4,
      size_t num_replication_threads = 4);

  void stop();

protected:



  void reopenStreamChunks();

  TSDBNodeRef noderef_;
  Vector<Pair<String, ScopedPtr<StreamConfig>>> configs_;
  std::mutex mutex_;
  HashMap<String, RefPtr<StreamChunk>> chunks_;
  Vector<RefPtr<CompactionWorker>> compaction_workers_;
  Vector<RefPtr<ReplicationWorker>> replication_workers_;
};

} // namespace tdsb

#endif
