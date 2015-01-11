/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_TRACKEDSESSION_H
#define _CM_TRACKEDSESSION_H
#include <mutex>
#include <stdlib.h>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include <fnord/base/datetime.h>

#include "itemref.h"
#include "trackedquery.h"

namespace cm {
class CustomerNamespace;

/**
 * The max time after which a click on a query result is considered a click
 */
static const uint64_t kMaxQueryClickDelaySeconds = 180;

/**
 * Flush/expire a session after N seconds of inactivity
 */
static const uint64_t kSessionIdleTimeoutSeconds = 60 * 90;

struct JoinedSession {
  std::string customer_key;
  std::vector<TrackedQuery> queries;
  std::vector<TrackedItemVisit> item_visits;

  template <typename T>
  static void reflect(T* meta) {
    meta->prop(&cm::JoinedSession::customer_key, 1, "customer", false);
    meta->prop(&cm::JoinedSession::queries, 2, "queries", false);
    meta->prop(&cm::JoinedSession::item_visits, 3, "item_visits", false);
  };
};

/**
 * A tracked session. Make sure to hold the mutex when updating or accessing
 * any of the fields.
 */
struct TrackedSession {
  std::string customer_key;
  std::unordered_map<std::string, TrackedQuery> queries;
  std::unordered_map<std::string, TrackedItemVisit> item_visits;
  uint64_t last_seen_unix_micros;

  /**
   * Trigger an update to incorporate new information. This will e.g. mark
   * query items as clicked if a corresponding click was observed.
   *
   * required precondition: must hold the session mutex
   */
  void update();
  void debugPrint(const std::string& uid) const;

  JoinedSession toJoinedSession() const;
};


} // namespace cm
#endif
