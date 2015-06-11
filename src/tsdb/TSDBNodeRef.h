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
#include <fnord/stdtypes.h>
#include <fnord/random.h>
#include <fnord/option.h>
#include <fnord/autoref.h>
#include <fnord/thread/queue.h>
#include <fnord/thread/DelayedQueue.h>
#include <fnord/thread/CoalescingDelayedQueue.h>
#include <fnord-mdb/MDB.h>
#include <fnord-dht/ReplicationScheme.h>
#include <fnord/http/httpconnectionpool.h>

using namespace fnord;

namespace tsdb {

class StreamChunk;

struct TSDBNodeRef {
  const String db_path;
  thread::CoalescingDelayedQueue<StreamChunk> compactionq;
  thread::CoalescingDelayedQueue<StreamChunk> replicationq;
  RefPtr<mdb::MDB> db;
  RefPtr<dht::ReplicationScheme> replication_scheme;
  http::HTTPConnectionPool* http;
};

} // namespace tdsb

#endif
