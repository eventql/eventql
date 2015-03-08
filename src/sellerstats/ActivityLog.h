/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_SELLERSTATSACTIVITYLOG_H
#define _CM_SELLERSTATSACTIVITYLOG_H
#include "fnord-base/stdtypes.h"
#include "fnord-rpc/RPC.h"
#include "fnord-rpc/RPCClient.h"
#include "fnord-base/thread/taskscheduler.h"
#include "fnord-mdb/MDB.h"
#include "fnord-base/stats/stats.h"
#include "fnord-json/json.h"
#include "ItemRef.h"
#include "JoinedItemVisit.h"
#include "FeatureIndex.h"
#include "FeatureSchema.h"

using namespace fnord;

namespace cm {
class CustomerNamespace;

typedef Pair<DateTime, json::JSONObject> ActivityLogEntry;

class ActivityLog {
public:

  Option<DateTime> fetchHead(
      const String& shopid,
      mdb::MDBTransaction* sellerstatsdb_txn);

  size_t fetch(
      const String& shopid,
      const DateTime& time,
      size_t limit,
      mdb::MDBTransaction* sellerstatsdb_txn,
      Vector<ActivityLogEntry>* entries);

  void append(
      const String& shopid,
      const DateTime& time,
      const json::JSONObject& entry,
      mdb::MDBTransaction* sellerstatsdb_txn);

};
} // namespace cm

#endif
