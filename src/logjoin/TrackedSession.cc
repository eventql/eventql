/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <fnord-base/inspect.h>
#include "logjoin/TrackedSession.h"

namespace cm {

/*
void TrackedSession::update() {
  // FIXPAUL slow slow slow
  for (const auto& visit : item_visits) {

    for (auto& query : queries) {
      auto tdiff = visit.time.unixMicros() - query.time.unixMicros();
      auto max_delay = kMaxQueryClickDelaySeconds * fnord::kMicrosPerSecond;
      if (query.time > visit.time || tdiff > max_delay) {
        continue;
      }

      for (auto& qitem : query.items) {
        if (visit.item == qitem.item) {
          qitem.clicked = true;
          break;
        }
      }
    }
  }
}
*/

DateTime TrackedSession::nextFlushTime() const {
  uint64_t res = last_seen_unix_micros +
      kSessionIdleTimeoutSeconds * fnord::kMicrosPerSecond;

  for (auto& query : queries) {
    auto qflush = query.time.unixMicros() +
        kMaxQueryClickDelaySeconds * fnord::kMicrosPerSecond;

    if (qflush < res) {
      res = qflush;
    }
  }

  for (auto& visit : item_visits) {
    auto vflush = visit.time.unixMicros() +
        kMaxQueryClickDelaySeconds * fnord::kMicrosPerSecond;

    if (vflush < res) {
      res = vflush;
    }
  }

  return DateTime(res);
}

void TrackedSession::debugPrint(const std::string& uid) const {
  fnord::iputs(
      ">> session uid=$0 last_seen=$1",
      uid,
      fnord::DateTime(last_seen_unix_micros));

  fnord::iputs(" > queries: ", 1);
  for (const auto& query : queries) {
    fnord::iputs(
        "    > query time=$0 eid=$2\n        > attrs: $1",
        query.time,
        query.attrs,
        query.eid);

    for (const auto& item : query.items) {
      fnord::iputs(
          "        > qitem: id=$0 clicked=$1 position=$2 variant=$3",
          item.item,
          item.clicked,
          item.position,
          item.variant);
    }
  }

  fnord::iputs(" > item visits: ", 1);
  for (const auto& view : item_visits) {
    fnord::iputs(
        "    > visit: item=$0 time=$1 eid=$3 attrs=$2",
        view.item,
        view.time,
        view.attrs,
        view.eid);
  }

  fnord::iputs("", 1);
}


/*
JoinedSession TrackedSession::toJoinedSession() const {
  JoinedSession sess;
  sess.customer_key = customer_key;

  for (const auto& p : queries) {
    sess.queries.emplace_back(p.second);
  }

  for (const auto& p : item_visits) {
    sess.item_visits.emplace_back(p.second);
  }

  return sess;
}
*/

} // namespace cm
