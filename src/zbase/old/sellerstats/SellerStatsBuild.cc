/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "sellerstats/SellerStatsBuild.h"
#include "sellerstats/SellerStatsItemVisit.h"
#include "CTRStatsKey.h"

using namespace stx;

namespace zbase {

SellerStatsBuild::SellerStatsBuild(
    FeatureSchema* feature_schema) :
    feature_index_(feature_schema),
    shop_id_feature_(feature_schema->featureID("shop_id").get()) {}

void SellerStatsBuild::insertJoinedItemVisit(
    const JoinedItemVisit& item_visit,
    mdb::MDBTransaction* sellerstatsdb_txn,
    mdb::MDBTransaction* featuredb_txn) {
  auto docid = item_visit.item.docID();

  Vector<CTRStatsObservation> ctr_stats;


  auto shopid = feature_index_.getFeature(
      docid,
      shop_id_feature_,
      featuredb_txn);

  if (shopid.isEmpty()) {
    stx::logWarning(
        "cm.sellerstats",
        "Item $0 not found in featuredb",
        docid.docid);

    return;
  }

#ifndef FNORD_NODEBUG
  stx::logDebug(
      "cm.sellerstats",
      "Indexing JoinedItemVisit\n    itemid=$0\n    shopid=$1\n    attrs=$2",
      inspect(item_visit.item),
      shopid.get(),
      inspect(item_visit.attrs));
#endif

  writeItemVisitToActivityLog(shopid.get(), item_visit, sellerstatsdb_txn);
}

void SellerStatsBuild::writeItemVisitToActivityLog(
      const String& shopid,
      const JoinedItemVisit& item_visit,
      mdb::MDBTransaction* sellerstatsdb_txn) {
  json::JSONObject json;
  json.emplace_back(json::JSON_OBJECT_BEGIN);
  json.emplace_back(json::JSON_STRING, "_t");
  json.emplace_back(json::JSON_STRING, "iv");
  json.emplace_back(json::JSON_STRING, "i");
  json.emplace_back(json::JSON_STRING, item_visit.item.docID().docid);
  json.emplace_back(json::JSON_STRING, "a");
  json::toJSON(item_visit.attrs, &json);
  json.emplace_back(json::JSON_OBJECT_END);
  activity_log_.append(shopid, item_visit.time, json, sellerstatsdb_txn);
}


} // namespace zbase

