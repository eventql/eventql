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
    String _data) :
    time(_time),
    evid(_evid),
    evtype(_evtype),
    data(_data) {}

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
  stx::iputs("* session $0/$1", customer_key, uid);
  for (const auto& ev : events) {
    stx::iputs(
        "    > event time=$0 evtype=$1 eid=$2 data=$3$4",
        ev.time,
        ev.evtype,
        ev.evid,
        ev.data.substr(0, 40),
        String(ev.data.size() > 40 ? "[...]" : ""));
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
