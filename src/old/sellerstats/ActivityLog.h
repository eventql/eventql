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
#include "fnord/stdtypes.h"
#include "fnord/rpc/RPC.h"
#include "fnord/rpc/RPCClient.h"
#include "fnord/thread/taskscheduler.h"
#include "fnord/mdb/MDB.h"
#include "fnord/stats/stats.h"
#include "fnord/json/json.h"
#include <inventory/ItemRef.h>
#include "JoinedItemVisit.h"
#include "FeatureIndex.h"


using namespace fnord;

namespace cm {
class CustomerNamespace;

typedef Pair<UnixTime, json::JSONObject> ActivityLogEntry;

class ActivityLog {
public:

  Option<UnixTime> fetchHead(
      const String& shopid,
      mdb::MDBTransaction* sellerstatsdb_txn);

  size_t fetch(
      const String& shopid,
      const UnixTime& time,
      size_t limit,
      mdb::MDBTransaction* sellerstatsdb_txn,
      Vector<ActivityLogEntry>* entries);

  void append(
      const String& shopid,
      const UnixTime& time,
      const json::JSONObject& entry,
      mdb::MDBTransaction* sellerstatsdb_txn);

};
} // namespace cm

#endif
