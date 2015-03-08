/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "index/IndexBuilder.h"
#include "IndexRequest.h"

using namespace fnord;

namespace cm {

IndexBuilder::IndexBuilder(
    const FeatureSchema* schema) :
    feature_schema_(schema),
    feature_index_(schema) {}

void IndexBuilder::indexDocument(
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

void IndexBuilder::updateFeatureIndex(
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
  feature_index_.updateFeatures(docid, features, txn);
}

} // namespace cm

