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
#include "fnord/comm/rpc.h"
#include "fnord/comm/rpcservicemap.h"
#include "fnord/thread/taskscheduler.h"
#include "itemref.h"
#include "trackedsession.h"
#include "trackedquery.h"

namespace cm {
class CustomerNamespace;

class LogJoinService {
public:

  LogJoinService(
      fnord::thread::TaskScheduler* scheduler,
      fnord::comm::RPCServiceMap* service_map);

  void insertLogline(
      CustomerNamespace* customer,
      const std::string& log_line);

  void insertLogline(
      CustomerNamespace* customer,
      const fnord::DateTime& time,
      const std::string& log_line);

protected:

  void recordJoinedQuery(
      CustomerNamespace* customer,
      const std::string& uid,
      const std::string& eid,
      const TrackedQuery& query);

  void recordJoinedItemVisit(
      CustomerNamespace* customer,
      const std::string& uid,
      const std::string& eid,
      const TrackedItemVisit& visit);

  void recordJoinedSession(
      CustomerNamespace* customer,
      const std::string& uid,
      const TrackedSession& session);

  void insertQuery(
      CustomerNamespace* customer,
      const std::string& uid,
      const std::string& eid,
      const TrackedQuery& query);

  void insertItemVisit(
      CustomerNamespace* customer,
      const std::string& uid,
      const std::string& eid,
      const TrackedItemVisit& visit);

  /**
   * Find or create a session with the provided user id
   *
   * postcondition: returns a tracked session with the mutex locked!
   */
  TrackedSession* findOrCreateSession(
      CustomerNamespace* customer,
      const std::string& uid);

  /**
   * Maybe flush a session
   *
   * precondition: the caller must hold the session's mutex
   */
  bool maybeFlushSession(
      const std::string uid,
      TrackedSession* session,
      const fnord::DateTime& stream_time);

  void flush(const fnord::DateTime& stream_time);

  fnord::DateTime streamTime(const fnord::DateTime& time);

  /**
   * Flush the session identified by the provided uid
   */
  //void flushSession(
  //    CustomerNamespace* customer,
  //    const std::string& uid);

  fnord::thread::TaskScheduler* scheduler_;
  fnord::comm::RPCChannel* logstream_channel_;
  std::unordered_map<std::string, std::unique_ptr<TrackedSession>> sessions_;
  std::mutex sessions_mutex_;
  fnord::DateTime stream_clock_;
  std::mutex stream_clock_mutex_;
};
} // namespace cm

#endif
