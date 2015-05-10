/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_TSDB_TSDBNODEREF_H
#define _FNORD_TSDB_TSDBNODEREF_H
#include <fnord-base/stdtypes.h>
#include <fnord-base/random.h>
#include <fnord-base/option.h>
#include <fnord-base/autoref.h>
#include <fnord-base/thread/queue.h>
#include <fnord-base/thread/DelayedQueue.h>
#include <fnord-mdb/MDB.h>
#include <fnord-dht/ReplicationScheme.h>

namespace fnord {
namespace tsdb {

class StreamChunk;

struct TSDBNodeRef {
  const String db_path;
  thread::DelayedQueue<RefPtr<StreamChunk>> compactionq;
  thread::Queue<RefPtr<StreamChunk>> replicationq;
  RefPtr<mdb::MDB> db;
  RefPtr<dht::ReplicationScheme> replication_scheme;
};

} // namespace tdsb
} // namespace fnord

#endif
