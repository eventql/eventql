/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <stx/inspect.h>
#include "zbase/logjoin/TrackedSession.h"
#include "common.h"

using namespace stx;

namespace zbase {

TrackedEvent::TrackedEvent(
    UnixTime _time,
    String _evid,
    String _evtype,
    String _data) :
    time(_time),
    evid(_evid),
    evtype(_evtype),
    data(_data) {}

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
      events.emplace_back(
          time,
          evid,
          "__sattr",
          URI::buildQueryString(logline));
      return;

    default:
      RAISE(kParseError, "invalid e param");

  }
}

void TrackedSession::encode(OutputStream* os) const {
  os->appendUInt8(0x1);

  os->appendLenencString(customer_key);
  os->appendLenencString(uuid);

  os->appendVarUInt(events.size());
  for (const auto& ev : events) {
    os->appendUInt64(ev.time.unixMicros());
    os->appendLenencString(ev.evid);
    os->appendLenencString(ev.evtype);
    os->appendLenencString(ev.data);
  }
}

void TrackedSession::decode(InputStream* is) {
  is->readUInt8();

  customer_key = is->readLenencString();
  uuid = is->readLenencString();

  auto nevents = is->readVarUInt();
  for (size_t i = 0; i < nevents; ++i) {
    auto time = is->readUInt64();
    auto evid = is->readLenencString();
    auto evtype = is->readLenencString();
    auto evdata = is->readLenencString();

    events.emplace_back(
        time,
        evid,
        evtype,
        evdata);
  }
}

} // namespace zbase
