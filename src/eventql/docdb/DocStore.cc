/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <eventql/util/fnv.h>
#include <eventql/util/io/file.h>
#include <eventql/util/json/json.h>
#include "DocStore.h"

using namespace stx;

namespace zbase {

DocStore::DocStore(const String& path) : path_(path) {}

RefPtr<Document> DocStore::updateDocument(const IndexChangeRequest& index_request) {
  std::unique_lock<std::mutex> lk(update_lock_);

  auto docid = index_request.item.docID();
  RefPtr<Document> doc(new Document(docid));
  loadDocument(doc);

  //doc->update(index_request);
  commitDocument(doc);

  return doc;
}

RefPtr<Document> DocStore::findDocument(const DocID& docid) {
  RefPtr<Document> doc(new Document(docid));
  loadDocument(doc);
  return doc;
}

void DocStore::loadDocument(RefPtr<Document> doc) {
  auto docpath = docPath(doc->docID());
  if (!FileUtil::exists(docpath)) {
    return;
  }

  auto doc_json = stx::json::parseJSON(FileUtil::read(docpath));
  for (int i = 1; i < doc_json.size() - 1; i += 2) {
    doc->setField(doc_json[i].data, doc_json[i + 1].data);
  }
}

void DocStore::commitDocument(RefPtr<Document> doc) {
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

String DocStore::docPath(DocID docid) const {
  stx::FNV<uint64_t> fnv;

  auto docid_str = docid.docid;
  auto h = fnv.hash(docid_str.c_str(), docid_str.length()) % 8192;
  StringUtil::replaceAll(&docid_str, "~", "_");

  return StringUtil::format("$0/$1/$2.json", path_, h, docid_str);
}

void DocStore::listDocuments(Function<bool (const DocID& doc)> fn) const {
  FileUtil::ls(path_, [this, fn] (const String& dirname) -> bool {
    auto dirpath = FileUtil::joinPaths(path_, dirname);
    auto ret = true;

    FileUtil::ls(dirpath, [this, fn, &ret] (const String& filename) -> bool {
      DocID docid = { .docid  = filename };

      StringUtil::replaceAll(&docid.docid, "_", "~");
      StringUtil::replaceAll(&docid.docid, ".json", "");

      if (fn(docid)) {
        return true;
      } else {
        ret = false;
        return false;
      }
    });

    return ret;
  });
}

} // namespace zbase

