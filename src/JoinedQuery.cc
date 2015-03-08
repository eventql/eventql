/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "JoinedQuery.h"
#include "common.h"

namespace cm {

Vector<Pair<String, double>> joinedQueryItemFeatures(
    const JoinedQuery& query,
    const JoinedQueryItem& item,
    cm::FeatureSchema* feature_schema,
    cm::FeatureIndex* feature_index,
    mdb::MDBTransaction* featuredb_txn) {
  Vector<Pair<String, double>> features;

  auto docid = item.item.docID();

  auto cat1_id = extractAttr(query.attrs, "q_cat1");

  auto shop_id = feature_index->getFeature(
      docid,
      feature_schema->featureID("shop_id").get(),
      featuredb_txn);

  /* cross cat1 id x shop id */
  if (!cat1_id.isEmpty() && !shop_id.isEmpty()) {
    auto f = StringUtil::format(
        "q_cat1:$0~shop_id:$1",
        cat1_id.get(),
        shop_id.get());

    features.emplace_back(f, 1.0);
  }

  return features;
}

}

