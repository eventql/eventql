/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <fnord-base/fnv.h>
#include <fnord-base/io/file.h>
#include <fnord-json/json.h>
#include "FullIndex.h"

using namespace fnord;

namespace cm {

FullIndex::FullIndex(const String& path) : path_(path) {}

RefPtr<Document> FullIndex::updateDocument(const IndexRequest& index_request) {
  std::unique_lock<std::mutex> lk(update_lock_);

  auto docid = index_request.item.docID();
  RefPtr<Document> doc(new Document(docid));
  loadDocument(doc);

  doc->update(index_request);
  commitDocument(doc);

  return doc;
}

RefPtr<Document> FullIndex::findDocument(const DocID& docid) {
  RefPtr<Document> doc(new Document(docid));
  loadDocument(doc);
  return doc;
}

void FullIndex::loadDocument(RefPtr<Document> doc) {
  auto docpath = docPath(doc->docID());
  if (!FileUtil::exists(docpath)) {
    return;
  }

  auto doc_json = fnord::json::parseJSON(FileUtil::read(docpath));
  for (int i = 1; i < doc_json.size() - 1; i += 2) {
    doc->setField(doc_json[i].data, doc_json[i + 1].data);
  }
}

void FullIndex::commitDocument(RefPtr<Document> doc) {
  auto docpath = docPath(doc->docID());
  auto docpath_tmp = docpath + "~";

  auto basepath = FileUtil::basePath(docpath);
  if (!FileUtil::exists(basepath)) {
    FileUtil::mkdir_p(basepath);
  }

  auto file = File::openFile(
      docpath_tmp,
      File::O_WRITE | File::O_CREATEOROPEN | File::O_TRUNCATE);

  auto doc_json = json::toJSONString(doc->fields());
  file.write(doc_json.data(), doc_json.size());

  FileUtil::mv(docpath_tmp, docpath);
}

String FullIndex::docPath(DocID docid) const {
  fnord::FNV<uint64_t> fnv;

  auto docid_str = docid.docid;
  auto h = fnv.hash(docid_str.c_str(), docid_str.length()) % 8192;
  StringUtil::replaceAll(&docid_str, "~", "_");

  return StringUtil::format("$0/$1/$2.json", path_, h, docid_str);
}

} // namespace cm

