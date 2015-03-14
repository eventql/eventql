/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "common.h"
#include "FeatureSelector.h"

using namespace fnord;

namespace cm {

FeatureSelector::FeatureSelector(
      RefPtr<mdb::MDB> featuredb,
      FeatureSchema* feature_schema) :
      featuredb_(featuredb),
      feature_schema_(feature_schema),
      feature_index_(feature_schema) {}

void FeatureSelector::featuresFor(
    const JoinedQuery& query,
    const JoinedQueryItem& item,
    Set<String>* features) {
  auto featuredb_txn = featuredb_->startTransaction(true);

  /* item attributes */
  auto docid = item.item.docID();

  auto shop_id = feature_index_.getFeature(
      docid,
      feature_schema_->featureID("shop_id").get(),
      featuredb_txn.get());

  if (!shop_id.isEmpty()) {
    features->emplace(StringUtil::format("shop_id:$0", shop_id.get()));
  }

  /* query attributes */
  auto cat1_id = extractAttr(query.attrs, "q_cat1");
  if (!cat1_id.isEmpty()) {
    features->emplace(StringUtil::format("q_cat1:$0", cat1_id.get()));
  }

  auto cat2_id = extractAttr(query.attrs, "q_cat2");
  if (!cat2_id.isEmpty()) {
    features->emplace(StringUtil::format("q_cat2:$0", cat2_id.get()));
  }

  auto cat3_id = extractAttr(query.attrs, "q_cat3");
  if (!cat3_id.isEmpty()) {
    features->emplace(StringUtil::format("q_cat3:$0", cat3_id.get()));
  }

  /* cross cat1 id x shop id */
  if (!cat1_id.isEmpty() && !shop_id.isEmpty()) {
    features->emplace(StringUtil::format(
        "q_cat1:$0|shop_id:$1",
        cat1_id.get(),
        shop_id.get()));
  }

  /* cross cat2 id x shop id */
  if (!cat2_id.isEmpty() && !shop_id.isEmpty()) {
    features->emplace(StringUtil::format(
        "q_cat2:$0|shop_id:$1",
        cat2_id.get(),
        shop_id.get()));
  }

  /* cross cat3 id x shop id */
  if (!cat3_id.isEmpty() && !shop_id.isEmpty()) {
    features->emplace(StringUtil::format(
        "q_cat3:$0|shop_id:$1",
        cat2_id.get(),
        shop_id.get()));
  }

  featuredb_txn->abort();
}

} // namespace cm

