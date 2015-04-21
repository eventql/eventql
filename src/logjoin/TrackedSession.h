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
#include <fnord-base/datetime.h>

#include "ItemRef.h"
#include "logjoin/TrackedQuery.h"
#include "logjoin/TrackedItemVisit.h"

using namespace fnord;

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

/**
 * A tracked session. Make sure to hold the mutex when updating or accessing
 * any of the fields.
 */
struct TrackedSession {
  std::string customer_key;
  std::string uid;
  std::vector<TrackedQuery> queries;
  std::vector<TrackedQuery> flushed_queries;
  std::vector<TrackedItemVisit> item_visits;
  uint64_t last_seen_unix_micros;
  bool flushed;
  std::vector<std::string> attrs;

  /**
   * Trigger an update to incorporate new information. This will e.g. mark
   * query items as clicked if a corresponding click was observed.
   *
   * required precondition: must hold the session mutex
   */
  void debugPrint(const std::string& uid) const;

  DateTime nextFlushTime() const;

  template <typename T>
  static void reflect(T* meta) {
    meta->prop(&cm::TrackedSession::customer_key, 1, "c", false);
    meta->prop(&cm::TrackedSession::uid, 2, "u", false);
    meta->prop(&cm::TrackedSession::queries, 3, "q", false);
    meta->prop(&cm::TrackedSession::flushed_queries, 8, "qf", false);
    meta->prop(&cm::TrackedSession::item_visits, 4, "v", false);
    meta->prop(&cm::TrackedSession::last_seen_unix_micros, 5, "t", false);
    meta->prop(&cm::TrackedSession::flushed, 6, "f", false);
    meta->prop(&cm::TrackedSession::attrs, 7, "a", false);
  };
};


} // namespace cm
#endif
