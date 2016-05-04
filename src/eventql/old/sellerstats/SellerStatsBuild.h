/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_SELLERSTATSBUILD_H
#define _CM_SELLERSTATSBUILD_H
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

#include "sellerstats/ActivityLog.h"

using namespace stx;

namespace zbase {
class CustomerNamespace;

class SellerStatsBuild {
public:

  SellerStatsBuild(FeatureSchema* feature_schema);

  void insertJoinedItemVisit(
      const JoinedItemVisit& item_visit,
      mdb::MDBTransaction* sellerstatsdb_txn,
      mdb::MDBTransaction* featuredb_txn);

  void exportStats(const std::string& path_prefix);

protected:

  void writeItemVisitToActivityLog(
      const String& shopid,
      const JoinedItemVisit& item_visit,
      mdb::MDBTransaction* sellerstatsdb_txn);

  ActivityLog activity_log_;
  FeatureIndex feature_index_;
  FeatureID shop_id_feature_;

  stx::stats::Counter<uint64_t> stat_processed_item_visits_;
};
} // namespace zbase

#endif
