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
#include <stx/UnixTime.h>
#include <stx/option.h>
#include <stx/Currency.h>

#include <inventory/ItemRef.h>
#include "logjoin/TrackedQuery.h"
#include "logjoin/TrackedItemVisit.h"
#include "logjoin/TrackedCartItem.h"
#include "logjoin/JoinedSession.pb.h"

using namespace stx;

namespace cm {

/**
 * The max time after which a click on a query result is considered a click
 */
static const uint64_t kMaxQueryClickDelaySeconds = 180;

/**
 * Flush/expire a session after N seconds of inactivity
 */
static const uint64_t kSessionIdleTimeoutSeconds = 60 * 90;

struct TrackedEvent {
  TrackedEvent(
      UnixTime _time,
      String _evid,
      String _evtype,
      String _data);

  UnixTime time;
  String evid;
  String evtype;
  String data;
};

/**
 * A tracked session. Make sure to hold the mutex when updating or accessing
 * any of the fields.
 */
struct TrackedSession {
  std::string customer_key;
  std::string uid;
  Vector<TrackedEvent> events;
  std::vector<std::string> attrs;

  void insertLogline(
      const UnixTime& time,
      const String& evtype,
      const String& evid,
      const URI::ParamList& logline);

  void debugPrint() const;

};

struct TrackedSessionContext : public RefCounted {

  TrackedSessionContext(TrackedSession session);

  TrackedSession tracked_session;

  JoinedSession joined_session;

};

} // namespace cm
#endif
