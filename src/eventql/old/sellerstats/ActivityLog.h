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
#include "eventql/util/stdtypes.h"
#include "eventql/util/rpc/RPC.h"
#include "eventql/util/rpc/RPCClient.h"
#include "eventql/util/thread/taskscheduler.h"
#include "eventql/util/mdb/MDB.h"
#include "eventql/util/stats/stats.h"
#include "eventql/util/json/json.h"
#include <eventql/docdb/ItemRef.h>
#include "JoinedItemVisit.h"
#include "FeatureIndex.h"


using namespace stx;

namespace zbase {
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
} // namespace zbase

#endif
