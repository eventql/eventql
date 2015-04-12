/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_LOGJOINSERVICE_H
#define _CM_LOGJOINSERVICE_H
#include <mutex>
#include <stdlib.h>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include <queue>
#include "fnord-base/stdtypes.h"
#include "fnord-feeds/RemoteFeed.h"
#include "fnord-feeds/RemoteFeedWriter.h"
#include "fnord-rpc/RPC.h"
#include "fnord-rpc/RPCClient.h"
#include "fnord-base/thread/taskscheduler.h"
#include "fnord-mdb/MDB.h"
#include "fnord-base/stats/stats.h"
#include "ItemRef.h"
#include "logjoin/TrackedSession.h"
#include "logjoin/TrackedQuery.h"
#include "logjoin/LogJoinShard.h"
#include "logjoin/LogJoinTarget.h"

using namespace fnord;

namespace cm {
class CustomerNamespace;

class LogJoin {
public:
  static const size_t kFlushIntervalMicros = 500 * fnord::kMicrosPerSecond;

  LogJoin(
      LogJoinShard shard,
      bool dry_run,
      LogJoinTarget* target);

  void insertLogline(
      const std::string& log_line,
      mdb::MDBTransaction* txn);

  void insertLogline(
      const std::string& customer_key,
      const fnord::DateTime& time,
      const std::string& log_line,
      mdb::MDBTransaction* txn);

  size_t numSessions() const;

  void flush(mdb::MDBTransaction* txn, DateTime stream_time);

  void importTimeoutList(mdb::MDBTransaction* txn);

  void exportStats(const std::string& path_prefix);

protected:

  void insertQuery(
      const std::string& customer_key,
      const std::string& uid,
      const TrackedQuery& query,
      mdb::MDBTransaction* txn);

  void insertItemVisit(
      const std::string& customer_key,
      const std::string& uid,
      const TrackedItemVisit& visit,
      mdb::MDBTransaction* txn);

  void withSession(
      const std::string& customer_key,
      const std::string& uid,
      mdb::MDBTransaction* txn,
      Function<void (TrackedSession* session)> fn);

  void enqueueFlush(
      const String& uid,
      const DateTime& flush_at);

  void maybeFlushSession(
      mdb::MDBTransaction* txn,
      const std::string uid,
      TrackedSession* session,
      DateTime stream_time);

  bool dry_run_;
  LogJoinShard shard_;
  LogJoinTarget* target_;
  HashMap<String, DateTime> sessions_flush_times_;

  fnord::stats::Counter<uint64_t> stat_loglines_total_;
  fnord::stats::Counter<uint64_t> stat_loglines_invalid_;
  fnord::stats::Counter<uint64_t> stat_joined_sessions_;
  fnord::stats::Counter<uint64_t> stat_joined_queries_;
  fnord::stats::Counter<uint64_t> stat_joined_item_visits_;
};
} // namespace cm

#endif
