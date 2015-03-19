/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "IndexBuild.h"

using namespace fnord;

namespace cm {

RefPtr<IndexBuild> IndexBuild::openIndex(const String& index_path) {
  if (!FileUtil::exists(index_path) || !FileUtil::isDirectory(index_path)) {
    RAISEF(kIllegalArgumentError, "invalid index path: $0", index_path);
  }

  /* set up feature schema */
  FeatureSchema feature_schema;
  feature_schema.registerFeature("shop_id", 1, 1);
  feature_schema.registerFeature("category1", 2, 1);
  feature_schema.registerFeature("category2", 3, 1);
  feature_schema.registerFeature("category3", 4, 1);
  feature_schema.registerFeature("title~de", 5, 2);

  /* open mdb */
  auto db_path = FileUtil::joinPaths(index_path, "db");
  FileUtil::mkdir_p(db_path);
  auto db = mdb::MDB::open(db_path);
  db->setMaxSize(1000000 * 512000);

  /* open docs store */
  auto docs_path = FileUtil::joinPaths(index_path, "docs");
  FileUtil::mkdir_p(docs_path);
  RefPtr<FullIndex> docs(new FullIndex(docs_path));

  return RefPtr<IndexBuild>(new IndexBuild(feature_schema, db, docs));
}


IndexBuild::IndexBuild(
    FeatureSchema schema,
    RefPtr<mdb::MDB> db,
    RefPtr<FullIndex> docs) :
    schema_(schema),
    db_(db),
    feature_idx_(new FeatureIndexWriter(&schema_)),
    docs_(docs) {}

void IndexBuild::updateDocument(const IndexRequest& index_request) {
  auto doc = docs_->updateDocument(index_request);
}

void IndexBuild::commit() {
}

void IndexBuild::rebuildFTS() {
  fnord::iputs("rebuild fts...", 1);

  docs_->listDocuments([this] (const DocID& docid) -> bool {
    fnord::iputs("rebuild fts for $0", docid.docid);
    return true;
  });
}

RefPtr<mdb::MDB> IndexBuild::featureDB() {
  return db_;
}

} // namespace cm
