/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "FeatureIndexWriter.h"
#include "IndexChangeRequest.h"

using namespace fnord;

namespace cm {

FeatureIndexWriter::FeatureIndexWriter(
    const String& db_path,
    bool readonly) :
    readonly_(readonly),
    db_(nullptr),
    txn_(nullptr) {
  db_ = mdb::MDB::open(db_path, false, 68719476736lu); // 64 GiB
  txn_ = db_->startTransaction(readonly_);

  /* set up feature schema */
  schema_.registerFeature("shop_id", 1, 1);
  schema_.registerFeature("category1", 2, 1);
  schema_.registerFeature("category2", 3, 1);
  schema_.registerFeature("category3", 4, 1);
  schema_.registerFeature("price_cents", 8, 1);

  schema_.registerFeature("title~de", 5, 2);
  schema_.registerFeature("description~de", 6, 2);
  schema_.registerFeature("size_description~de", 14, 2);
  schema_.registerFeature("material_description~de", 15, 2);
  schema_.registerFeature("basic_attributes~de", 16, 2);
  schema_.registerFeature("tags_as_text~de", 7, 2);
  schema_.registerFeature("title~pl", 18, 2);
  schema_.registerFeature("description~pl", 19, 2);
  schema_.registerFeature("size_description~pl", 20, 2);
  schema_.registerFeature("material_description~pl", 21, 2);
  schema_.registerFeature("basic_attributes~pl", 22, 2);
  schema_.registerFeature("tags_as_text~pl", 23, 2);
  schema_.registerFeature("image_filename", 24, 2);
  schema_.registerFeature("cm_clicked_terms", 31, 2);

  schema_.registerFeature("shop_name", 26, 3);
  schema_.registerFeature("shop_platform", 27, 3);
  schema_.registerFeature("shop_country", 28, 3);
  schema_.registerFeature("shop_rating_alt", 9, 3);
  schema_.registerFeature("shop_rating_alt2", 15, 3);
  schema_.registerFeature("shop_products_count", 10, 3);
  schema_.registerFeature("shop_orders_count", 11, 3);
  schema_.registerFeature("shop_rating_count", 12, 3);
  schema_.registerFeature("shop_rating_avg", 13, 3);
  schema_.registerFeature("cm_views", 29, 3);
  schema_.registerFeature("cm_clicks", 30, 3);
  schema_.registerFeature("cm_ctr", 32, 3);
  schema_.registerFeature("cm_ctr_norm", 33, 3);
  schema_.registerFeature("cm_ctr_std", 34, 3);
  schema_.registerFeature("cm_ctr_norm_std", 35, 3);
}

FeatureIndexWriter::~FeatureIndexWriter() {
  if (txn_.get()) {
    txn_->commit();
  }
}

void FeatureIndexWriter::commit() {
  txn_->commit();
  txn_ = db_->startTransaction(readonly_);
}

RefPtr<mdb::MDBTransaction> FeatureIndexWriter::dbTransaction() {
  return txn_;
}

void FeatureIndexWriter::updateDocument(const IndexChangeRequest& index_request) {
  logDebug(
      "cm.indexbuild",
      "Indexing document: customer=$0 docid=$1 num_attrs=$2",
      index_request.customer,
      index_request.item.docID().docid,
      index_request.attrs.size());

  updateIndex(index_request);
}

RefPtr<Document> FeatureIndexWriter::findDocument(const DocID& docid) {
  RefPtr<Document> doc(new Document(docid));

  FeaturePack features;
  for (const auto& group : schema_.groupIDs()) {
    auto db_key = featureDBKey(docid, group);
    auto buf = txn_->get(db_key);

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
    auto fkey = schema_.featureKey(f.first);
    if (fkey.isEmpty()) {
      continue;
    }

    doc->setField(fkey.get(), f.second);
  }

  return doc;
}

void FeatureIndexWriter::updateIndex(const IndexChangeRequest& index_request) {
  Vector<Pair<FeatureID, String>> features;
  for (const auto& p : index_request.attrs) {
    auto fid = schema_.featureID(p.first);

    if (fid.isEmpty()) {
      continue;
    }

    features.emplace_back(fid.get(), p.second);
  }

  auto docid = index_request.item.docID();
  updateIndex(docid, features);
}

void FeatureIndexWriter::updateIndex(
    const DocID& docid,
    const Vector<Pair<FeatureID, String>>& features) {
  Set<uint64_t> groups;
  for (const auto& p : features) {
    groups.emplace(p.first.group);
  }

  for (const auto& group : groups) {
    FeaturePack group_features;
    auto db_key = featureDBKey(docid, group);

    auto old_buf = txn_->get(db_key);
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

    txn_->update(db_key, pack.data(), pack.size());
  }
}

void FeatureIndexWriter::listDocuments(Function<bool (const DocID& id)> fn) {
  Buffer key;
  Buffer value;
  String last_key;

  auto cursor = txn_->getCursor();
  for (int n = 0; ; ) {
    bool eof;
    if (n++ == 0) {
      eof = !cursor->getFirst(&key, &value);
    } else {
      eof = !cursor->getNext(&key, &value);
    }

    if (eof) {
      break;
    }

    auto key_str = key.toString();
    if (StringUtil::beginsWith(key_str, "__")) {
      continue;
    }

    key_str.erase(key_str.find("~", 3), std::string::npos);
    if (key_str == last_key) {
      continue;
    }

    last_key = key_str;
    if (!fn(DocID { .docid = key_str })) {
      break;
    }
  }

  cursor->close();
}

void FeatureIndexWriter::getFields(const DocID& docid, FeaturePack* features) {
  for (const auto& group : schema_.groupIDs()) {
    auto db_key = featureDBKey(docid, group);
    auto buf = txn_->get(db_key);

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

Option<String> FeatureIndexWriter::getField(
    const DocID& docid,
    const String& feature) {
  auto fid = schema_.featureID(feature);
  if (fid.isEmpty()) {
    RAISEF(kIndexError, "unknown feature: $0", feature);
  }

  return getField(docid, fid.get());
}

Option<String> FeatureIndexWriter::getField(
    const DocID& docid,
    const FeatureID& featureid) {
  auto db_key = featureDBKey(docid, featureid.group);

  //auto cached = cache_.get(db_key);
  //if (!cached.isEmpty()) {
  //  return cached;
  //}

  auto buf = txn_->get(db_key);

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


} // namespace cm

