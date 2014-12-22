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
#include "itemref.h"
#include "trackedsession.h"
#include "trackedquery.h"

namespace cm {
class CustomerNamespace;

class LogJoinService {
public:

  LogJoinService();

  void insertLogline(
      CustomerNamespace* customer,
      const std::string& log_line);

protected:

  void insertLogline(
      CustomerNamespace* customer,
      const fnord::DateTime& time,
      const std::string& log_line);

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

  TrackedSession* findOrCreateSession(
      CustomerNamespace* customer,
      const std::string& uid);

  void flushTrackedQuery(const std::string& uid, const TrackedQuery& query);
  void flushSession(const std::string& uid, const TrackedSession& session);

  std::unordered_map<std::string, std::unique_ptr<TrackedSession>> sessions_;
  std::mutex sessions_mutex_;
};

} // namespace cm
#endif
