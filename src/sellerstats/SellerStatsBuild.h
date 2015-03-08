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
#include "sellerstats/ActivityLog.h"

using namespace fnord;

namespace cm {
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

  fnord::stats::Counter<uint64_t> stat_processed_item_visits_;
};
} // namespace cm

#endif
