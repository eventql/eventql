/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_DOCSTORE_H
#define _CM_DOCSTORE_H
#include "stx/stdtypes.h"
#include "stx/rpc/RPC.h"
#include "stx/rpc/RPCClient.h"
#include "stx/thread/taskscheduler.h"
#include "stx/mdb/MDB.h"
#include "stx/stats/stats.h"
#include <stx/autoref.h>
#include <inventory/ItemRef.h>
#include "Document.h"
#include "IndexChangeRequest.h"

using namespace stx;

namespace zbase {

class DocStore : public RefCounted {
public:

  DocStore(const String& path);

  RefPtr<Document> updateDocument(const IndexChangeRequest& index_request);
  RefPtr<Document> findDocument(const DocID& docid);

  void listDocuments(Function<bool (const DocID& doc)> fn) const;

protected:

  String docPath(DocID docid) const;

  void loadDocument(RefPtr<Document> doc);
  void commitDocument(RefPtr<Document> doc);

  std::mutex update_lock_;
  String path_;
};

} // namespace zbase

#endif
