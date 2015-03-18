/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <fnord-base/fnv.h>
#include "FullIndex.h"

using namespace fnord;

namespace cm {

FullIndex::FullIndex(const String& path) : path_(path) {}

RefPtr<Document> FullIndex::updateDocument(const IndexRequest& index_request) {
  std::unique_lock<std::mutex> lk(update_lock_);

  auto docid = index_request.item.docID();
  auto docpath = docPath(docid);

  RefPtr<Document> doc(new Document(docid));

  if (FileUtil::exists(docpath)) {
    fnord::iputs("loaddoc: $0", docpath);
  } else {
    fnord::iputs("newdoc: $0", docpath);
  }

  doc->debugPrint();
  doc->update(index_request);

  return doc;
}

String FullIndex::docPath(DocID docid) const {
  fnord::FNV<uint64_t> fnv;

  auto docid_str = docid.docid;
  auto h = fnv.hash(docid_str.c_str(), docid_str.length()) % 8192;
  StringUtil::replaceAll(&docid_str, "~", "_");

  return StringUtil::format("$0/$1/$2.json", path_, h, docid_str);
}

} // namespace cm

