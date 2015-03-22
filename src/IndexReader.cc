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
  feature_schema.registerFeature("price_cents", 8, 1);

  feature_schema.registerFeature("title~de", 5, 2);
  feature_schema.registerFeature("description~de", 6, 2);
  feature_schema.registerFeature("size_description~de", 14, 2);
  feature_schema.registerFeature("material_description~de", 15, 2);
  feature_schema.registerFeature("basic_attributes~de", 16, 2);
  feature_schema.registerFeature("tags_as_text~de", 7, 2);
  feature_schema.registerFeature("title~pl", 18, 2);
  feature_schema.registerFeature("description~pl", 19, 2);
  feature_schema.registerFeature("size_description~pl", 20, 2);
  feature_schema.registerFeature("material_description~pl", 21, 2);
  feature_schema.registerFeature("basic_attributes~pl", 22, 2);
  feature_schema.registerFeature("tags_as_text~pl", 23, 2);
  feature_schema.registerFeature("image_filename", 24, 2);

  feature_schema.registerFeature("shop_name", 26, 3);
  feature_schema.registerFeature("shop_platform", 27, 3);
  feature_schema.registerFeature("shop_country", 28, 3);
  feature_schema.registerFeature("shop_rating_alt", 9, 3);
  feature_schema.registerFeature("shop_rating_alt2", 15, 3);
  feature_schema.registerFeature("shop_products_count", 10, 3);
  feature_schema.registerFeature("shop_orders_count", 11, 3);
  feature_schema.registerFeature("shop_rating_count", 12, 3);
  feature_schema.registerFeature("shop_rating_avg", 13, 3);

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
    fts_searcher_(new fnord::fts::IndexSearcher(fts_)),
    doc_idx_(new FeatureIndexWriter(&schema_)) {}


IndexReader::~IndexReader() {
  fts_->close();
}

std::shared_ptr<fts::IndexSearcher> IndexReader::ftsSearcher() {
  return fts_searcher_;
}

RefPtr<FeatureIndexWriter> IndexReader::docIndex() {
  return doc_idx_;
}

} // namespace cm
