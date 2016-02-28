/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "IndexWriter.h"
#include <zbase/util/AnalyzerAdapter.h>

using namespace stx;

namespace zbase {

RefPtr<IndexWriter> IndexWriter::openIndex(
    const String& index_path,
    const String& conf_path) {
  if (!FileUtil::exists(index_path) || !FileUtil::isDirectory(index_path)) {
    RAISEF(kIllegalArgumentError, "invalid index path: $0", index_path);
  }

  /* open doc idx */
  auto db_path = FileUtil::joinPaths(index_path, "db");
  FileUtil::mkdir_p(db_path);
  auto doc_idx = RefPtr<DocIndex>(
      new DocIndex(index_path, "documents-dawanda", false));

  /* open lucene */
  RefPtr<stx::fts::Analyzer> analyzer(new stx::fts::Analyzer(conf_path));
  auto adapter = std::make_shared<stx::fts::AnalyzerAdapter>(analyzer);

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

  return RefPtr<IndexWriter>(new IndexWriter(doc_idx, fts));
}

IndexWriter::IndexWriter(
    RefPtr<DocIndex> doc_idx,
    std::shared_ptr<fts::IndexWriter> fts_idx) :
    doc_idx_(doc_idx),
    fts_idx_(fts_idx) {}

IndexWriter::~IndexWriter() {
  fts_idx_->close();
}

void IndexWriter::updateDocument(const IndexChangeRequest& index_request) {
  stat_documents_indexed_total_.incr(1);
  auto docid = index_request.item.docID();
  doc_idx_->updateDocument(index_request);
  auto doc = doc_idx_->findDocument(docid);
  rebuildFTS(doc);
  stat_documents_indexed_success_.incr(1);
}

void IndexWriter::commit() {
  doc_idx_->commit();
  fts_idx_->commit();
}

void IndexWriter::rebuildFTS(DocID docid) {
  auto doc = doc_idx_->findDocument(docid);
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

  double boost = 0.7;
  double cm_clicks = 0;
  double cm_views = 0;
  double cm_ctr_norm_std = 1.0;

  bool is_active = false;

  HashMap<String, String> fts_fields_anal;
  for (const auto& f : doc->fields()) {

    /* title~LANG */
    if (StringUtil::beginsWith(f.first, "title~")) {
      is_active = true;
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

  if (cm_views > 1000 && cm_clicks > 15) {
    boost = cm_ctr_norm_std;
  }

  fts_doc->setBoost(boost);

  stx::logDebug(
      "cm.indexwriter",
      "Rebuilding FTS Index for docid=$0 boost=$1 active=$2",
      doc->docID().docid,
      boost,
      is_active);

  auto del_term = fts::newLucene<fts::Term>(
      L"_docid",
      StringUtil::convertUTF8To16(doc->docID().docid));

  if (is_active) {
    fts_idx_->updateDocument(del_term, fts_doc);
  } else {
    fts_idx_->deleteDocuments(del_term);
  }
}

void IndexWriter::rebuildFTS(size_t csize) {
  Vector<DocID> docids;

  doc_idx_->listDocuments([&docids] (const DocID& docid) -> bool {
    docids.emplace_back(docid);
    return true;
  });

  size_t n;
  for (const auto& docid : docids) {
    rebuildFTS(docid);

    if (++n > csize) {
      commit();
      n = 0;
    }
  }
}

RefPtr<mdb::MDBTransaction> IndexWriter::dbTransaction() {
  return doc_idx_->dbTransaction();
}

void IndexWriter::exportStats(const String& prefix) {
  exportStat(
      StringUtil::format("$0/documents_indexed_total", prefix),
      &stat_documents_indexed_total_,
      stx::stats::ExportMode::EXPORT_DELTA);

  exportStat(
      StringUtil::format("$0/documents_indexed_success", prefix),
      &stat_documents_indexed_success_,
      stx::stats::ExportMode::EXPORT_DELTA);

  exportStat(
      StringUtil::format("$0/documents_indexed_error", prefix),
      &stat_documents_indexed_error_,
      stx::stats::ExportMode::EXPORT_DELTA);

  exportStat(
      StringUtil::format("$0/documents_indexed_fts", prefix),
      &stat_documents_indexed_fts_,
      stx::stats::ExportMode::EXPORT_DELTA);
}


} // namespace zbase
