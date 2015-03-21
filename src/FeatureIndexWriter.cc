/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "FeatureIndexWriter.h"
#include "IndexRequest.h"

using namespace fnord;

namespace cm {

FeatureIndexWriter::FeatureIndexWriter(
    const FeatureSchema* schema) :
    feature_schema_(schema) {}

void FeatureIndexWriter::updateDocument(
    const IndexRequest& index_request,
    mdb::MDBTransaction* txn) {
  logDebug(
      "cm.indexbuild",
      "Indexing document: customer=$0 docid=$1 num_attrs=$2",
      index_request.customer,
      index_request.item.docID().docid,
      index_request.attrs.size());

  updateFeatureIndex(index_request, txn);
}

RefPtr<Document> FeatureIndexWriter::findDocument(
    const DocID& docid,
    mdb::MDBTransaction* featuredb_txn) {
  RefPtr<Document> doc(new Document(docid));

  FeaturePack features;
  for (const auto& group : feature_schema_->groupIDs()) {
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
    pack.readFeatures(&features);
  }

  for (const auto& f : features) {
    auto fkey = feature_schema_->featureKey(f.first);
    if (fkey.isEmpty()) {
      continue;
    }

    doc->setField(fkey.get(), f.second);
  }

  return doc;
}

void FeatureIndexWriter::updateFeatureIndex(
    const IndexRequest& index_request,
    mdb::MDBTransaction* txn) {
  Vector<Pair<FeatureID, String>> features;
  for (const auto& p : index_request.attrs) {
    auto fid = feature_schema_->featureID(p.first);

    if (fid.isEmpty()) {
      continue;
    }

    features.emplace_back(fid.get(), p.second);
  }

  auto docid = index_request.item.docID();
  updateFeatureIndex(docid, features, txn);
}

void FeatureIndexWriter::updateFeatureIndex(
    const DocID& docid,
    const Vector<Pair<FeatureID, String>>& features,
    mdb::MDBTransaction* featuredb_txn) {
  Set<uint64_t> groups;
  for (const auto& p : features) {
    groups.emplace(p.first.group);
  }

  for (const auto& group : groups) {
    FeaturePack group_features;
    auto db_key = featureDBKey(docid, group);

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

