/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_FEATUREINDEX_H
#define _CM_FEATUREINDEX_H
#include "fnord-base/stdtypes.h"
#include "fnord-rpc/RPC.h"
#include "fnord-rpc/RPCClient.h"
#include "fnord-base/thread/taskscheduler.h"
#include "fnord-mdb/MDB.h"
#include "fnord-base/stats/stats.h"
#include "FeatureCache.h"
#include "FeaturePack.h"
#include "FeatureSchema.h"
#include "FeatureID.h"
#include "ItemRef.h"

using namespace fnord;

namespace cm {

class FeatureIndex {
public:

  FeatureIndex(const FeatureSchema* schema);

  void getFeatures(
      const DocID& docid,
      mdb::MDBTransaction* featuredb_txn,
      FeaturePack* features);

  Option<String> getFeature(
      const DocID& docid,
      const FeatureID& featureid,
      mdb::MDBTransaction* featuredb_txn);

  void updateFeatures(
      const DocID& docid,
      const Vector<Pair<FeatureID, String>>& features,
      mdb::MDBTransaction* featuredb_txn);

protected:

  String dbKey(const DocID& docid, uint64_t group_id) const;

  FeatureCache cache_;
  const FeatureSchema* schema_;
};
} // namespace cm

#endif
