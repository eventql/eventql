/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "FeatureCache.h"
#include "FeatureIndex.h"
#include "FeaturePack.h"

using namespace fnord;

namespace cm {

FeatureIndex::FeatureIndex(
    RefPtr<mdb::MDB> featuredb,
    const FeatureSchema* schema) :
    db_(featuredb),
    schema_(schema) {}

void FeatureIndex::getFeatures(
    const DocID& docid,
    mdb::MDBTransaction* featuredb_txn,
    FeaturePack* features) {
  for (const auto& group : schema_->groupIDs()) {
    auto db_key = featureDBKey(docid, group);
    auto buf = featuredb_txn->get(db_key);

#ifndef FNORD_NOTRACE
  fnord::logTrace(
      "cm.featureindex",
      "Read from featuredb with key=$0 returned $1 bytes",
      db_key,
      buf.isEmpty() ? 0 : buf.get().size());
#endif

    if (buf.isEmpty()) {
      continue;
    }

    FeaturePackReader pack(buf.get().data(), buf.get().size());
    pack.readFeatures(features);
  }
}

Option<String> FeatureIndex::getFeature(
    const DocID& docid,
    const FeatureID& featureid,
    mdb::MDBTransaction* featuredb_txn) {
  auto db_key = featureDBKey(docid, featureid.group);

  //auto cached = cache_.get(db_key);
  //if (!cached.isEmpty()) {
  //  return cached;
  //}

  auto buf = featuredb_txn->get(db_key);

#ifndef FNORD_NOTRACE
fnord::logTrace(
    "cm.featureindex",
    "Read from featuredb with key=$0 returned $1 bytes",
    db_key,
    buf.isEmpty() ? 0 : buf.get().size());
#endif

  if (buf.isEmpty()) {
    return None<String>();
  }

  FeaturePack features;
  FeaturePackReader pack(buf.get().data(), buf.get().size());
  pack.readFeatures(&features);

  for (const auto& f : features) {
    if (f.first.feature == featureid.feature) {
      //cache_.store(db_key, f.second);
      return Some(f.second);
    }
  }

  return None<String>();
}

String featureDBKey(const DocID& docid, uint64_t group_id) {
  return StringUtil::format("$0~$1", docid.docid, group_id);
}

} // namespace cm
