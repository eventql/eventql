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
} // namespace cm

#endif
