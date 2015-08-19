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
#include "stx/stdtypes.h"
#include "brokerd/RemoteFeed.h"
#include "brokerd/RemoteFeedWriter.h"
#include "brokerd/RemoteFeedFactory.h"
#include "brokerd/RemoteFeedReader.h"
#include "stx/rpc/RPC.h"
#include "stx/rpc/RPCClient.h"
#include "stx/thread/taskscheduler.h"
#include "zbase/util/mdb/MDB.h"
#include "stx/stats/stats.h"
#include "stx/thread/eventloop.h"
#include <zbase/docdb/ItemRef.h>
#include "zbase/logjoin/TrackedSession.h"
#include "zbase/logjoin/TrackedQuery.h"
#include "zbase/logjoin/LogJoinShard.h"
#include "zbase/logjoin/SessionProcessor.h"

using namespace stx;

namespace zbase {
class CustomerNamespace;

/**
 * Flush/expire a session after N seconds of inactivity
 */
static const uint64_t kSessionIdleTimeoutSeconds = 60 * 90;

class LogJoin {
public:
  static const size_t kFlushIntervalMicros = 500 * stx::kMicrosPerSecond;

  LogJoin(
      LogJoinShard shard,
      bool dry_run,
      RefPtr<mdb::MDB> sessdb,
      SessionProcessor* target,
      thread::EventLoop* evloop);

  void processClickstream(
      const HashMap<String, URI>& input_feeds,
      size_t batch_size,
      size_t buffer_size,
      Duration flush_interval);

  void shutdown();

  size_t numSessions() const;
  size_t cacheSize() const;

  void exportStats(const std::string& path_prefix);

protected:

  void addPixelParamID(const String& param, uint32_t id);
  uint32_t getPixelParamID(const String& param) const;
  const String& getPixelParamName(uint32_t id) const;

  void insertLogline(
      const std::string& log_line,
      mdb::MDBTransaction* txn);

  void insertLogline(
      const std::string& customer_key,
      const stx::UnixTime& time,
      const std::string& log_line,
      mdb::MDBTransaction* txn);

  void appendToSession(
      const std::string& customer_key,
      const stx::UnixTime& time,
      const std::string& uid,
      const std::string& evid,
      const std::string& evtype,
      const Vector<Pair<String, String>>& logline,
      mdb::MDBTransaction* txn);

  void flush(mdb::MDBTransaction* txn, UnixTime stream_time);

  void flushSession(
      const std::string uid,
      UnixTime stream_time,
      mdb::MDBTransaction* txn);

  void importTimeoutList(mdb::MDBTransaction* txn);

  LogJoinShard shard_;
  bool dry_run_;
  RefPtr<mdb::MDB> sessdb_;
  SessionProcessor* target_;

  std::atomic<bool> shutdown_;

  HTTPRPCClient rpc_client_;
  feeds::RemoteFeedReader feed_reader_;

  HashMap<String, UnixTime> sessions_flush_times_;
  HashMap<String, TrackedSession> session_cache_;

  HashMap<String, uint32_t> pixel_param_ids_;
  HashMap<uint32_t, String> pixel_param_names_;

  stx::stats::Counter<uint64_t> stat_loglines_total_;
  stx::stats::Counter<uint64_t> stat_loglines_invalid_;
  stx::stats::Counter<uint64_t> stat_joined_sessions_;
  stx::stats::Counter<uint64_t> stat_joined_queries_;
  stx::stats::Counter<uint64_t> stat_joined_item_visits_;
  stx::stats::Counter<uint64_t> stat_stream_time_low;
  stx::stats::Counter<uint64_t> stat_stream_time_high;
  stx::stats::Counter<uint64_t> stat_active_sessions;
  stx::stats::Counter<uint64_t> stat_dbsize;

  Random rnd_;
};
} // namespace zbase

#endif
