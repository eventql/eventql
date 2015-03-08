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
    const FeatureSchema* schema) :
    schema_(schema) {}

String FeatureIndex::dbKey(const DocID& docid, uint64_t group_id) const {
  return StringUtil::format("$0~$1", docid.docid, group_id);
}

void FeatureIndex::getFeatures(
    const DocID& docid,
    mdb::MDBTransaction* featuredb_txn,
    FeaturePack* features) {
  for (const auto& group : schema_->groupIDs()) {
    auto db_key = dbKey(docid, group);
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
  auto db_key = dbKey(docid, featureid.group);

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

void FeatureIndex::updateFeatures(
    const DocID& docid,
    const Vector<Pair<FeatureID, String>>& features,
    mdb::MDBTransaction* featuredb_txn) {
  Set<uint64_t> groups;
  for (const auto& p : features) {
    groups.emplace(p.first.group);
  }

  for (const auto& group : groups) {
    FeaturePack group_features;
    auto db_key = dbKey(docid, group);

    auto old_buf = featuredb_txn->get(db_key);
    if (!old_buf.isEmpty()) {
      FeaturePackReader old_pack(old_buf.get().data(), old_buf.get().size());
      old_pack.readFeatures(&group_features);
    }

    for (const auto& p : features) {
      if (p.first.group != group) {
        continue;
      }

      bool found = false;
      for (auto& gp : group_features) {
        if (gp.first.feature == p.first.feature) {
          gp.second = p.second;
          found = true;
          break;
        }
      }

      if (!found) {
        group_features.emplace_back(p.first, p.second);
      }
    }

    sortFeaturePack(group_features);

    FeaturePackWriter pack;
    pack.writeFeatures(group_features);

    /* fastpath for noop updates */
    if (!old_buf.isEmpty() &&
        pack.size() == old_buf.get().size() &&
        memcmp(old_buf.get().data(), pack.data(), pack.size()) == 0) {
      continue;
    }

    featuredb_txn->update(db_key, pack.data(), pack.size());
  }
}

} // namespace cm
