/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <stx/inspect.h>
#include "logjoin/TrackedSession.h"
#include "common.h"

using namespace stx;

namespace cm {

TrackedEvent::TrackedEvent(
    UnixTime _time,
    String _evid,
    String _evtype,
    String _value) :
    time(_time),
    evid(_evid),
    evtype(_evtype),
    value(_value) {}

TrackedSession::TrackedSession() :
  num_cart_items(0),
  num_order_items(0),
  gmv_eurcents(0),
  cart_value_eurcents(0) {}

void TrackedSession::insertLogline(
    const UnixTime& time,
    const String& evtype,
    const String& evid,
    const URI::ParamList& logline) {
  if (evtype.length() != 1) {
    RAISE(kParseError, "e param invalid");
  }

  switch (evtype[0]) {

    /* query event */
    case 'q':
      events.emplace_back(
          time,
          evid,
          "_search_query",
          URI::buildQueryString(logline));
      return;

    case 'v':
      events.emplace_back(
          time,
          evid,
          "_pageview",
          URI::buildQueryString(logline));
      return;

    case 'c':
      events.emplace_back(
          time,
          evid,
          "_cart_items",
          URI::buildQueryString(logline));
      return;

    case 'u':
      updateSessionAttributes(time, evid, logline);
      return;

    default:
      RAISE(kParseError, "invalid e param");

  }
}

void TrackedSession::updateSessionAttributes(
    const UnixTime& time,
    const String& evid,
    const URI::ParamList& logline) {
  for (const auto& p : logline) {
    if (p.first == "x") {
      experiments.emplace(p.second);
      continue;
    }
  }

  std::string r_url;
  if (stx::URI::getParam(logline, "r_url", &r_url)) {
    referrer_url = Some(r_url);
  }

  std::string r_cpn;
  if (stx::URI::getParam(logline, "r_cpn", &r_cpn)) {
    referrer_campaign = Some(r_cpn);
  }

  std::string r_nm;
  if (stx::URI::getParam(logline, "r_nm", &r_nm)) {
    referrer_name = Some(r_nm);
  }

  std::string cs;
  if (stx::URI::getParam(logline, "cs", &cs)) {
    customer_session_id = Some(cs);
  }
}

void TrackedSession::debugPrint() const {
  stx::iputs("=== session $0/$1 ===", customer_key, uid);

  stx::iputs(" > queries: ", 1);
  for (const auto& query : queries) {
    stx::iputs(
        "    > query time=$0 eid=$2\n        > attrs: $1",
        query.time,
        query.attrs,
        query.eid);

    for (const auto& item : query.items) {
      stx::iputs(
          "        > qitem: id=$0 clicked=$1 position=$2 variant=$3",
          item.item,
          item.clicked,
          item.position,
          item.variant);
    }
  }

  stx::iputs(" > item visits: ", 1);
  for (const auto& view : item_visits) {
    stx::iputs(
        "    > visit: item=$0 time=$1 eid=$3 attrs=$2",
        view.item,
        view.time,
        view.attrs,
        view.eid);
  }

  stx::iputs("", 1);
}

Option<UnixTime> TrackedSession::firstSeenTime() const {
  uint64_t t = std::numeric_limits<uint64_t>::max();

  for (const auto& e : queries) {
    if (e.time.unixMicros() < t) {
      t = e.time.unixMicros();
    }
  }

  for (const auto& e : item_visits) {
    if (e.time.unixMicros() < t) {
      t = e.time.unixMicros();
    }
  }

  for (const auto& e : cart_items) {
    if (e.time.unixMicros() < t) {
      t = e.time.unixMicros();
    }
  }

  if (t == std::numeric_limits<uint64_t>::max()) {
    return None<UnixTime>();
  } else {
    return Some(UnixTime(t));
  }
}

Option<UnixTime> TrackedSession::lastSeenTime() const {
  uint64_t t = std::numeric_limits<uint64_t>::min();

  for (const auto& e : queries) {
    if (e.time.unixMicros() > t) {
      t = e.time.unixMicros();
    }
  }

  for (const auto& e : item_visits) {
    if (e.time.unixMicros() > t) {
      t = e.time.unixMicros();
    }
  }

  for (const auto& e : cart_items) {
    if (e.time.unixMicros() > t) {
      t = e.time.unixMicros();
    }
  }

  if (t == std::numeric_limits<uint64_t>::min()) {
    return None<UnixTime>();
  } else {
    return Some(UnixTime(t));
  }
}

String TrackedSession::joinedExperiments() const {
  String joined;

  for (const auto& e : experiments) {
    joined += e;
    joined += ';';
  }

  return joined;
}

} // namespace cm
