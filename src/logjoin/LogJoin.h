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
#include "stx/rpc/RPC.h"
#include "stx/rpc/RPCClient.h"
#include "stx/thread/taskscheduler.h"
#include "stx/mdb/MDB.h"
#include "stx/stats/stats.h"
#include <inventory/ItemRef.h>
#include "logjoin/TrackedSession.h"
#include "logjoin/TrackedQuery.h"
#include "logjoin/LogJoinShard.h"
#include "logjoin/SessionProcessor.h"

using namespace stx;

namespace cm {
class CustomerNamespace;

class LogJoin {
public:
  static const size_t kFlushIntervalMicros = 500 * stx::kMicrosPerSecond;

  LogJoin(
      LogJoinShard shard,
      bool dry_run,
      SessionProcessor* target);

  void insertLogline(
      const std::string& log_line,
      mdb::MDBTransaction* txn);

  void insertLogline(
      const std::string& customer_key,
      const stx::UnixTime& time,
      const std::string& log_line,
      mdb::MDBTransaction* txn);

  size_t numSessions() const;
  size_t cacheSize() const;

  void flush(mdb::MDBTransaction* txn, UnixTime stream_time);

  void importTimeoutList(mdb::MDBTransaction* txn);

  void exportStats(const std::string& path_prefix);

protected:

  void addPixelParamID(const String& param, uint32_t id);
  uint32_t getPixelParamID(const String& param) const;
  const String& getPixelParamName(uint32_t id) const;

  //void insertQuery(
  //    const std::string& customer_key,
  //    const std::string& uid,
  //    const TrackedQuery& query,
  //    mdb::MDBTransaction* txn);

  //void insertItemVisit(
  //    const std::string& customer_key,
  //    const std::string& uid,
  //    const TrackedItemVisit& visit,
  //    mdb::MDBTransaction* txn);

  //void insertCartVisit(
  //    const std::string& customer_key,
  //    const std::string& uid,
  //    const Vector<TrackedCartItem>& cart_items,
  //    const UnixTime& time,
  //    mdb::MDBTransaction* txn);

  void appendToSession(
      const std::string& customer_key,
      const stx::UnixTime& time,
      const std::string& uid,
      const std::string& evid,
      const std::string& evtype,
      const Vector<Pair<String, String>>& logline,
      mdb::MDBTransaction* txn);

  void flushSession(
      const std::string uid,
      UnixTime stream_time,
      mdb::MDBTransaction* txn);

  //void onSession(
  //    mdb::MDBTransaction* txn,
  //    TrackedSession& session);

  bool dry_run_;
  LogJoinShard shard_;
  SessionProcessor* target_;
  HashMap<String, UnixTime> sessions_flush_times_;
  HashMap<String, TrackedSession> session_cache_;

  HashMap<String, uint32_t> pixel_param_ids_;
  HashMap<uint32_t, String> pixel_param_names_;

  stx::stats::Counter<uint64_t> stat_loglines_total_;
  stx::stats::Counter<uint64_t> stat_loglines_invalid_;
  stx::stats::Counter<uint64_t> stat_joined_sessions_;
  stx::stats::Counter<uint64_t> stat_joined_queries_;
  stx::stats::Counter<uint64_t> stat_joined_item_visits_;

  Random rnd_;
};
} // namespace cm

#endif
