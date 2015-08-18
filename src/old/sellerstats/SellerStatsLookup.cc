/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "sellerstats/ActivityLog.h"
#include "sellerstats/SellerStatsLookup.h"

using namespace stx;

namespace zbase {

String SellerStatsLookup::lookup(
    const String& shopid,
    const String& cmdata_path,
    const String& customer) {
  /* set up feature schema */
  FeatureSchema feature_schema;
  feature_schema.registerFeature("shop_id", 1, 1);
  feature_schema.registerFeature("category1", 2, 1);
  feature_schema.registerFeature("category2", 3, 1);
  feature_schema.registerFeature("category3", 4, 1);

  /* open featuredb db */
  auto featuredb_path = FileUtil::joinPaths(
      cmdata_path,
      StringUtil::format("index/$0/db", customer));

  auto featuredb = mdb::MDB::open(featuredb_path, true);
  auto featuredb_txn = featuredb->startTransaction(true);

  /* open sellerstats db */
  auto sellerstatsdb_path = FileUtil::joinPaths(
      cmdata_path,
      StringUtil::format("sellerstats/$0/db", customer));

  FileUtil::mkdir_p(sellerstatsdb_path);
  auto sellerstatsdb = mdb::MDB::open(sellerstatsdb_path, true);
  auto sellerstatsdb_txn = sellerstatsdb->startTransaction(true);

  auto res= lookup(
      shopid,
      featuredb_txn.get(),
      sellerstatsdb_txn.get());

  featuredb_txn->abort();
  sellerstatsdb_txn->abort();

  return res;

}

String SellerStatsLookup::lookup(
    const String& shopid,
    mdb::MDBTransaction* featuredb_txn,
    mdb::MDBTransaction* sellerstatsdb_txn) {
  /* fetch activity log */
  ActivityLog activity_log;
  auto activity_log_head = activity_log.fetchHead(
      shopid,
      sellerstatsdb_txn);

  Vector<ActivityLogEntry> activity_log_entries;
  if (!activity_log_head.isEmpty()) {
    activity_log.fetch(
        shopid,
        activity_log_head.get(),
        100,
        sellerstatsdb_txn,
        &activity_log_entries);
  }

  /* dump document */
  String out;

  out += StringUtil::format(
      "[sellerstats]\n  shopid=$0\n\n[activity log]\n",
      shopid);

  for (const auto& e : activity_log_entries) {
    out += StringUtil::format(
        "    $0 => $1\n",
        e.first,
        json::toJSONString(e.second));
  }

  return out;
}

} // namespace zbase

