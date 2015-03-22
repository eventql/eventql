/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "IndexWriter.h"
#include <fnord-fts/AnalyzerAdapter.h>

using namespace fnord;

namespace cm {

RefPtr<IndexWriter> IndexWriter::openIndex(
    const String& index_path,
    const String& conf_path) {
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
  feature_schema.registerFeature("cm_clicked_terms", 31, 2);

  feature_schema.registerFeature("shop_name", 26, 3);
  feature_schema.registerFeature("shop_platform", 27, 3);
  feature_schema.registerFeature("shop_country", 28, 3);
  feature_schema.registerFeature("shop_rating_alt", 9, 3);
  feature_schema.registerFeature("shop_rating_alt2", 15, 3);
  feature_schema.registerFeature("shop_products_count", 10, 3);
  feature_schema.registerFeature("shop_orders_count", 11, 3);
  feature_schema.registerFeature("shop_rating_count", 12, 3);
  feature_schema.registerFeature("shop_rating_avg", 13, 3);
  feature_schema.registerFeature("cm_views", 29, 3);
  feature_schema.registerFeature("cm_clicks", 30, 3);
  feature_schema.registerFeature("cm_ctr", 32, 3);
  feature_schema.registerFeature("cm_ctr_norm", 33, 3);
  feature_schema.registerFeature("cm_ctr_std", 34, 3);
  feature_schema.registerFeature("cm_ctr_norm_std", 35, 3);

  /* open mdb */
  auto db_path = FileUtil::joinPaths(index_path, "db");
  FileUtil::mkdir_p(db_path);
  auto db = mdb::MDB::open(db_path, false, 68719476736lu); // 64 GiB

  /* open lucene */
  RefPtr<fnord::fts::Analyzer> analyzer(new fnord::fts::Analyzer(conf_path));
  auto adapter = std::make_shared<fnord::fts::AnalyzerAdapter>(analyzer);

  auto fts_path = FileUtil::joinPaths(index_path, "fts");
  bool create = false;
  if (!FileUtil::exists(fts_path)) {
    FileUtil::mkdir_p(fts_path);
    create = true;
  }

  auto fts =
      fts::newLucene<fts::IndexWriter>(
          fts::FSDirectory::open(StringUtil::convertUTF8To16(fts_path)),
          adapter,
          create,
          fts::IndexWriter::MaxFieldLengthLIMITED);

  return RefPtr<IndexWriter>(new IndexWriter(feature_schema, db, fts));
}

IndexWriter::IndexWriter(
    FeatureSchema schema,
    RefPtr<mdb::MDB> db,
    std::shared_ptr<fts::IndexWriter> fts) :
    schema_(schema),
    db_(db),
    db_txn_(db_->startTransaction()),
    feature_idx_(new FeatureIndexWriter(&schema_)),
    fts_(fts) {}


IndexWriter::~IndexWriter() {
  if (db_txn_.get()) {
    db_txn_->commit();
  }

  fts_->close();
}

void IndexWriter::updateDocument(const IndexRequest& index_request) {
  stat_documents_indexed_total_.incr(1);
  auto docid = index_request.item.docID();
  feature_idx_->updateDocument(index_request, db_txn_.get());
  auto doc = feature_idx_->findDocument(docid, db_txn_.get());
  rebuildFTS(doc);
  stat_documents_indexed_success_.incr(1);
}

void IndexWriter::commit() {
  db_txn_->commit();
  db_txn_ = db_->startTransaction();
  fts_->commit();
}

void IndexWriter::rebuildFTS(DocID docid) {
  auto doc = feature_idx_->findDocument(docid, db_txn_.get());
  doc->debugPrint();
  rebuildFTS(doc);
}

void IndexWriter::rebuildFTS(RefPtr<Document> doc) {
  stat_documents_indexed_fts_.incr(1);
  auto fts_doc = fts::newLucene<fts::Document>();

  fts_doc->add(
      fts::newLucene<fts::Field>(
          L"_docid",
          StringUtil::convertUTF8To16(doc->docID().docid),
          fts::Field::STORE_YES,
          fts::Field::INDEX_NOT_ANALYZED_NO_NORMS));

  double boost = 1.0;
  double cm_clicks = 0;
  double cm_views = 0;
  double cm_ctr_norm_std = 1.0;

  HashMap<String, String> fts_fields_anal;
  for (const auto& f : doc->fields()) {

    /* title~LANG */
    if (StringUtil::beginsWith(f.first, "title~")) {
      auto k = f.first;
      StringUtil::replaceAll(&k, "title~","title~");
      fts_fields_anal[k] += " " + f.second;
    }

    /* description~LANG */
    else if (StringUtil::beginsWith(f.first, "description~")) {
      auto k = f.first;
      StringUtil::replaceAll(&k, "description~","text~");
      fts_fields_anal[k] += " " + f.second;
    }

    /* size_description~LANG */
    else if (StringUtil::beginsWith(f.first, "size_description~")) {
      auto k = f.first;
      StringUtil::replaceAll(&k, "size_description~","text~");
      fts_fields_anal[k] += " " + f.second;
    }

    /* material_description~LANG */
    else if (StringUtil::beginsWith(f.first, "material_description~")) {
      auto k = f.first;
      StringUtil::replaceAll(&k, "material_description~","text~");
      fts_fields_anal[k] += " " + f.second;
    }

    /* manufacturing_description~LANG */
    else if (StringUtil::beginsWith(f.first, "manufacturing_description~")) {
      auto k = f.first;
      StringUtil::replaceAll(&k, "manufacturing_description~","text~");
      fts_fields_anal[k] += " " + f.second;
    }

    /* tags_as_text~LANG */
    else if (StringUtil::beginsWith(f.first, "tags_as_text~")) {
      fts_fields_anal["tags"] += " " + f.second;
    }

    /* shop_name */
    else if (f.first == "shop_name") {
      fts_fields_anal["tags"] += " " + f.second;
    }

    /* cm_clicked_terms */
    else if (f.first == "cm_clicked_terms") {
      fts_doc->add(
          fts::newLucene<fts::Field>(
              L"cm_clicked_terms",
              StringUtil::convertUTF8To16(f.second),
              fts::Field::STORE_NO,
              fts::Field::INDEX_ANALYZED));
    }

    else if (f.first == "cm_ctr_norm_std") {
      cm_ctr_norm_std = std::stod(f.second);
    }

    else if (f.first == "cm_clicks") {
      cm_clicks = std::stod(f.second);
    }

    else if (f.first == "cm_views") {
      cm_views = std::stod(f.second);
    }
  }

  for (const auto& f : fts_fields_anal) {
    fts_doc->add(
        fts::newLucene<fts::Field>(
            StringUtil::convertUTF8To16(f.first),
            StringUtil::convertUTF8To16(f.second),
            fts::Field::STORE_NO,
            fts::Field::INDEX_ANALYZED));
  }

  if (cm_views > 2000) {
    boost = cm_ctr_norm_std;
  }

  fts_doc->setBoost(boost);

  fnord::logDebug(
      "cm.indexwriter",
      "Rebuilding FTS Index for docid=$0 boost=$1",
      doc->docID().docid,
      boost);

  auto del_term = fts::newLucene<fts::Term>(
      L"_docid",
      StringUtil::convertUTF8To16(doc->docID().docid));

  fts_->updateDocument(del_term, fts_doc);
}

void IndexWriter::rebuildFTS() {
  //docs_->listDocuments([this] (const DocID& docid) -> bool {
  //  rebuildFTS(docid);
  //  return true;
  //});
}

RefPtr<mdb::MDBTransaction> IndexWriter::dbTransaction() {
  return db_txn_;
}

void IndexWriter::exportStats(const String& prefix) {
  exportStat(
      StringUtil::format("$0/documents_indexed_total", prefix),
      &stat_documents_indexed_total_,
      fnord::stats::ExportMode::EXPORT_DELTA);

  exportStat(
      StringUtil::format("$0/documents_indexed_success", prefix),
      &stat_documents_indexed_success_,
      fnord::stats::ExportMode::EXPORT_DELTA);

  exportStat(
      StringUtil::format("$0/documents_indexed_error", prefix),
      &stat_documents_indexed_error_,
      fnord::stats::ExportMode::EXPORT_DELTA);

  exportStat(
      StringUtil::format("$0/documents_indexed_fts", prefix),
      &stat_documents_indexed_fts_,
      fnord::stats::ExportMode::EXPORT_DELTA);
}


} // namespace cm
