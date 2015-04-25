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

using namespace fnord;

namespace cm {

void TrackedSession::update() {
  /* update queries */
  for (auto& cur_query : queries) {

    /* search for matching item visits */
    for (auto& cur_visit : item_visits) {
      auto cutoff = cur_query.time.unixMicros() +
          kMaxQueryClickDelaySeconds * kMicrosPerSecond;

      if (cur_visit.time < cur_query.time ||
          cur_visit.time.unixMicros() > cutoff) {
        continue;
      }

      for (auto& qitem : cur_query.items) {
        if (cur_visit.item == qitem.item) {
          qitem.clicked = true;
          break;
        }
      }
    }

  }
}

void TrackedSession::insertLogline(
    const DateTime& time,
    const String& evtype,
    const String& evid,
    const URI::ParamList& logline) {
  if (evtype.length() != 1) {
    RAISE(kParseError, "e param invalid");
  }

  switch (evtype[0]) {

    /* query event */
    case 'q': {
      TrackedQuery query;
      query.time = time;
      query.eid = evid;
      query.fromParams(logline);
      insertQuery(query);
      break;
    }

    /* item visit event */
    case 'v': {
      TrackedItemVisit visit;
      visit.time = time;
      visit.eid = evid;
      visit.fromParams(logline);
      insertItemVisit(visit);
      break;
    }

    /* cart event */
    case 'c': {
      auto cart_items = TrackedCartItem::fromParams(logline);
      for (auto& ci : cart_items) {
        ci.time = time;
      }

      insertCartVisit(cart_items);
      break;
    }

    case 'u':
      return;

    default:
      RAISE(kParseError, "invalid e param");

  }
}

void TrackedSession::insertQuery(const TrackedQuery& query) {
  for (auto& q : queries) {
    if (q.eid == query.eid) {
      q.merge(query);
      return;
    }
  }

  queries.emplace_back(query);
}

void TrackedSession::insertItemVisit(const TrackedItemVisit& visit) {
  for (auto& v : item_visits) {
    if (v.eid == visit.eid) {
      v.merge(visit);
      return;
    }
  }

  item_visits.emplace_back(visit);
}

void TrackedSession::insertCartVisit(
    const Vector<TrackedCartItem>& new_cart_items) {
  for (const auto& cart_item : new_cart_items) {
    bool merged = false;

    for (auto& c : cart_items) {
      if (c.item == cart_item.item) {
        c.merge(cart_item);
        merged = true;
        break;
      }
    }

    if (!merged) {
      cart_items.emplace_back(cart_item);
    }
  }
}

void TrackedSession::debugPrint(const std::string& uid) const {
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
