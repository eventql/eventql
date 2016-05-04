/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "sellerstats/ActivityLog.h"

using namespace stx;

namespace zbase {

void ActivityLog::append(
    const String& shopid,
    const UnixTime& time,
    const json::JSONObject& entry,
    mdb::MDBTransaction* sellerstatsdb_txn) {
  auto json_str = json::toJSONString(entry);
  auto timestamp = time.unixMicros() / kMicrosPerSecond;
  auto entry_dbkey = StringUtil::format("activitylog~$0~$1", shopid, timestamp);
  auto head_dbkey = StringUtil::format("activitylog~$0~HEAD", shopid);

#ifndef FNORD_NOTRACE
  stx::logTrace(
      "cm.sellerstats",
      "Write to activity log: shopid=$0 time=$1 dbkey=$2 => $3",
      shopid,
      time,
      entry_dbkey,
      json_str);
#endif

  sellerstatsdb_txn->insert(entry_dbkey, json_str);

  auto head_time = fetchHead(shopid, sellerstatsdb_txn);
  if (head_time.isEmpty() || time > head_time.get()) {
    sellerstatsdb_txn->update(head_dbkey, StringUtil::toString(timestamp));
  }
}

Option<UnixTime> ActivityLog::fetchHead(
    const String& shopid,
    mdb::MDBTransaction* sellerstatsdb_txn) {
  auto head_dbkey = StringUtil::format("activitylog~$0~HEAD", shopid);

  auto buf = sellerstatsdb_txn->get(head_dbkey);
  if (buf.isEmpty()) {
    return None<UnixTime>();
  } else {
    return Some(UnixTime(std::stoul(buf.get().toString()) * kMicrosPerSecond));
  }
}

size_t ActivityLog::fetch(
    const String& shopid,
    const UnixTime& time,
    size_t limit,
    mdb::MDBTransaction* sellerstatsdb_txn,
    Vector<ActivityLogEntry>* entries) {
  auto cur = sellerstatsdb_txn->getCursor();

  auto dbkey_prefix = StringUtil::format("activitylog~$0~", shopid);
  auto dbkey_head = StringUtil::format("$0HEAD", dbkey_prefix);
  auto dbkey = StringUtil::format(
      "$0$1",
      dbkey_prefix,
      time.unixMicros() / kMicrosPerSecond);

  Buffer entry;
  Buffer dbkey_buf;
  if (!cur->get(dbkey, &entry)) {
    RAISEF(kRuntimeError, "entry not found: $0", dbkey);
  }

  size_t n;
  for (n = 1; true; ++n) {
    auto json = json::parseJSON(entry);
    auto entry_time_iter = json::JSONUtil::objectLookup(
        json.begin(),
        json.end(),
        "_t");

    if (entry_time_iter == json.end() ||
        entry_time_iter->type != json::JSON_STRING) {
      RAISEF(kRuntimeError, "entry has no _t field: $0", entry.toString());
    }

    auto entry_time = UnixTime(
        std::stoul(dbkey.substr(dbkey_prefix.length())) * kMicrosPerSecond);

    entries->emplace_back(entry_time, json);

    if (!cur->getPrev(&dbkey_buf, &entry)) {
      break;
    }

    dbkey = dbkey_buf.toString();

    if (!StringUtil::beginsWith(dbkey, dbkey_prefix) || dbkey == dbkey_head) {
      break;
    }
  }

  return n;
}

} // namespace zbase

