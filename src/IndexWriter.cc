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
  feature_schema.registerFeature("title~de", 5, 2);

  /* open mdb */
  auto db_path = FileUtil::joinPaths(index_path, "db");
  FileUtil::mkdir_p(db_path);
  auto db = mdb::MDB::open(db_path);
  db->setMaxSize(1000000 * 512000);

  /* open docs store */
  auto docs_path = FileUtil::joinPaths(index_path, "docs");
  FileUtil::mkdir_p(docs_path);
  RefPtr<DocStore> docs(new DocStore(docs_path));

  /* open lucene */
  RefPtr<fnord::fts::Analyzer> analyzer(new fnord::fts::Analyzer(conf_path));
  auto adapter = std::make_shared<fnord::fts::AnalyzerAdapter>(analyzer);

  auto fts_path = FileUtil::joinPaths(index_path, "fts");
  auto fts =
      fts::newLucene<fts::IndexWriter>(
          fts::FSDirectory::open(StringUtil::convertUTF8To16(fts_path)),
          adapter,
          true,
          fts::IndexWriter::MaxFieldLengthLIMITED);

  return RefPtr<IndexWriter>(new IndexWriter(feature_schema, db, docs, fts));
}

IndexWriter::IndexWriter(
    FeatureSchema schema,
    RefPtr<mdb::MDB> db,
    RefPtr<DocStore> docs,
    std::shared_ptr<fts::IndexWriter> fts) :
    schema_(schema),
    db_(db),
    feature_idx_(new FeatureIndexWriter(&schema_)),
    docs_(docs),
    fts_(fts) {}


IndexWriter::~IndexWriter() {
  fts_->close();
}

void IndexWriter::updateDocument(const IndexRequest& index_request) {
  stat_documents_indexed_total_.incr(1);
  auto doc = docs_->updateDocument(index_request);
  rebuildFTS(doc);
  stat_documents_indexed_success_.incr(1);
}

void IndexWriter::commit() {
  fts_->commit();
}

void IndexWriter::rebuildFTS(DocID docid) {
  auto doc = docs_->findDocument(docid);
  rebuildFTS(doc);
}

void IndexWriter::rebuildFTS(RefPtr<Document> doc) {
  stat_documents_indexed_fts_.incr(1);
  auto fts_doc = fts::newLucene<fts::Document>();

  fnord::logDebug(
      "cm.indexwriter",
      "Rebuilding FTS Index for docid=$0",
      doc->docID().docid);

  HashMap<String, String> fts_fields_anal;
  for (const auto& f : doc->fields()) {

    /* title~LANG */
    if (StringUtil::beginsWith(f.first, "title~")) {
      auto k = f.first;
      StringUtil::replaceAll(&k, "title~","title~");
      fts_fields_anal[k] += " ";
      fts_fields_anal[k] += f.second;
    }

    /* description~LANG */
    if (StringUtil::beginsWith(f.first, "description~")) {
      auto k = f.first;
      StringUtil::replaceAll(&k, "description~","text~");
      fts_fields_anal[k] += " ";
      fts_fields_anal[k] += f.second;
    }

    /* tags_as_text~LANG */
    if (StringUtil::beginsWith(f.first, "tags_as_text~")) {
      auto k = f.first;
      StringUtil::replaceAll(&k, "tags_as_text~","text~");
      fts_fields_anal[k] += " ";
      fts_fields_anal[k] += f.second;
    }

  }

  fts_doc->add(
        fts::newLucene<fts::Field>(
            L"docid",
            StringUtil::convertUTF8To16(doc->docID().docid),
            fts::Field::STORE_YES,
            fts::Field::INDEX_NO));

  for (const auto& f : fts_fields_anal) {
    fts_doc->add(
        fts::newLucene<fts::Field>(
            StringUtil::convertUTF8To16(f.first),
            StringUtil::convertUTF8To16(f.second),
            fts::Field::STORE_NO,
            fts::Field::INDEX_ANALYZED));
  }

  fts_->addDocument(fts_doc);
}

void IndexWriter::rebuildFTS() {
  docs_->listDocuments([this] (const DocID& docid) -> bool {
    rebuildFTS(docid);
    return true;
  });
}

RefPtr<mdb::MDB> IndexWriter::featureDB() {
  return db_;
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
