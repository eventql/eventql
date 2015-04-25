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
#include "logjoin/TrackedCartItem.h"

using namespace fnord;

namespace cm {

/**
 * The max time after which a click on a query result is considered a click
 */
static const uint64_t kMaxQueryClickDelaySeconds = 180;

/**
 * Flush/expire a session after N seconds of inactivity
 */
static const uint64_t kSessionIdleTimeoutSeconds = 60;// * 90;

/**
 * A tracked session. Make sure to hold the mutex when updating or accessing
 * any of the fields.
 */
struct TrackedSession {
  std::string customer_key;
  std::string uid;
  std::vector<TrackedQuery> queries;
  std::vector<TrackedItemVisit> item_visits;
  std::vector<TrackedCartItem> cart_items;
  std::vector<std::string> attrs;

  void insertLogline(
      const DateTime& time,
      const String& evtype,
      const String& evid,
      const URI::ParamList& logline);

  /**
   * Trigger an update to incorporate new information. This will e.g. mark
   * query items as clicked if a corresponding click was observed.
   */
  void update();

  void debugPrint(const std::string& uid) const;

};


} // namespace cm
#endif
