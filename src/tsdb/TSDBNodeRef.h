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
#include <stx/stdtypes.h>
#include <stx/random.h>
#include <stx/option.h>
#include <stx/autoref.h>
#include <stx/thread/queue.h>
#include <stx/thread/DelayedQueue.h>
#include <stx/thread/CoalescingDelayedQueue.h>
#include <stx/mdb/MDB.h>
#include <dproc/ReplicationScheme.h>
#include <stx/http/httpconnectionpool.h>

using namespace fnord;

namespace tsdb {

class Partition;

struct TSDBNodeRef {
  TSDBNodeRef(
      const String& _db_path,
      RefPtr<mdb::MDB> _db,
      RefPtr<dproc::ReplicationScheme> _replication_scheme,
      http::HTTPConnectionPool* _http) :
      db_path(_db_path),
      db(_db),
      replication_scheme(_replication_scheme),
      http(_http) {}

  const String db_path;
  thread::CoalescingDelayedQueue<Partition> compactionq;
  thread::CoalescingDelayedQueue<Partition> replicationq;
  RefPtr<mdb::MDB> db;
  RefPtr<dproc::ReplicationScheme> replication_scheme;
  http::HTTPConnectionPool* http;
};

} // namespace tdsb

#endif
