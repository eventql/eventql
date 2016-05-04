/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_SELLERSTATSLOOKUP_H
#define _CM_SELLERSTATSLOOKUP_H
#include "stx/stdtypes.h"
#include "stx/rpc/RPC.h"
#include "stx/rpc/RPCClient.h"
#include "stx/thread/taskscheduler.h"
#include "eventql/util/mdb/MDB.h"
#include "stx/stats/stats.h"
#include "stx/json/json.h"
#include <eventql/docdb/ItemRef.h>
#include "JoinedItemVisit.h"
#include "FeatureIndex.h"


using namespace stx;

namespace zbase {
class CustomerNamespace;

class SellerStatsLookup {
public:

  static String lookup(
      const String& shopid,
      mdb::MDBTransaction* featuredb_txn,
      mdb::MDBTransaction* sellerstatsdb_txn);

  static String lookup(
      const String& shopid,
      const String& cmdata_path,
      const String& customer);

};
} // namespace zbase

#endif
