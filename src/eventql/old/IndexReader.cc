/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "IndexReader.h"
#include <eventql/util/AnalyzerAdapter.h>

using namespace stx;

namespace zbase {

RefPtr<IndexReader> IndexReader::openIndex(const String& index_path) {
  if (!FileUtil::exists(index_path) || !FileUtil::isDirectory(index_path)) {
    RAISEF(kIllegalArgumentError, "invalid index path: $0", index_path);
  }

  /* open doc index */
  auto db_path = FileUtil::joinPaths(index_path, "db");
  auto doc_idx = RefPtr<DocIndex>(
      new DocIndex(db_path, "documents-dawanda", true));

  /* open lucene */
  auto fts_path = FileUtil::joinPaths(index_path, "fts");
  auto fts = fts::IndexReader::open(
      fts::FSDirectory::open(StringUtil::convertUTF8To16(fts_path)),
      true);

  return RefPtr<IndexReader>(new IndexReader(doc_idx, fts));
}

IndexReader::IndexReader(
    RefPtr<DocIndex> doc_idx,
    std::shared_ptr<fts::IndexReader> fts) :
    doc_idx_(doc_idx),
    fts_(fts),
    fts_searcher_(new stx::fts::IndexSearcher(fts_)) {}

IndexReader::~IndexReader() {
  fts_->close();
}

std::shared_ptr<fts::IndexSearcher> IndexReader::ftsSearcher() {
  return fts_searcher_;
}

RefPtr<DocIndex> IndexReader::docIndex() {
  return doc_idx_;
}

} // namespace zbase
