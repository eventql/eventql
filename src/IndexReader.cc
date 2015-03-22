/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "IndexReader.h"
#include <fnord-fts/AnalyzerAdapter.h>

using namespace fnord;

namespace cm {

RefPtr<IndexReader> IndexReader::openIndex(const String& index_path) {
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
  auto db = mdb::MDB::open(db_path, true);

  /* open lucene */
  auto fts_path = FileUtil::joinPaths(index_path, "fts");
  auto fts = fts::IndexReader::open(
      fts::FSDirectory::open(StringUtil::convertUTF8To16(fts_path)),
      true);

  return RefPtr<IndexReader>(new IndexReader(feature_schema, db, fts));
}

IndexReader::IndexReader(
    FeatureSchema schema,
    RefPtr<mdb::MDB> db,
    std::shared_ptr<fts::IndexReader> fts) :
    schema_(schema),
    db_(db),
    fts_(fts),
    fts_searcher_(new fnord::fts::IndexSearcher(fts_)) {}


IndexReader::~IndexReader() {
  fts_->close();
}

} // namespace cm
