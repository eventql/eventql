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

  FeatureIndex(
      RefPtr<mdb::MDB> featuredb,
      const FeatureSchema* schema);

  ~FeatureIndex();

  FeatureIndex(const FeatureIndex& other) = delete;
  FeatureIndex& operator=(const FeatureIndex& other) = delete;

  Option<String> getFeature(
      const DocID& docid,
      const String& feature);

  Option<String> getFeature(
      const DocID& docid,
      const FeatureID& featureid);

  Option<String> getFeature(
      const DocID& docid,
      const FeatureID& featureid,
      mdb::MDBTransaction* featuredb_txn);

  void getFeatures(
      const DocID& docid,
      mdb::MDBTransaction* featuredb_txn,
      FeaturePack* features);

  void getFeatures(
      const DocID& docid,
      FeaturePack* features);

protected:

  FeatureCache cache_;
  RefPtr<mdb::MDB> db_;
  RefPtr<mdb::MDBTransaction> txn_;
  const FeatureSchema* schema_;
};

String featureDBKey(const DocID& docid, uint64_t group_id);

} // namespace cm

#endif
