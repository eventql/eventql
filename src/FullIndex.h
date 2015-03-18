/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_FULLINDEX_H
#define _CM_FULLINDEX_H
#include "fnord-base/stdtypes.h"
#include "fnord-rpc/RPC.h"
#include "fnord-rpc/RPCClient.h"
#include "fnord-base/thread/taskscheduler.h"
#include "fnord-mdb/MDB.h"
#include "fnord-base/stats/stats.h"
#include <fnord-base/autoref.h>
#include "ItemRef.h"
#include "Document.h"
#include "IndexRequest.h"

using namespace fnord;

namespace cm {

class FullIndex {
public:

  FullIndex(const String& path);

  RefPtr<Document> updateDocument(const IndexRequest& index_request);
  RefPtr<Document> findDocument(const DocID& docid);

protected:

  String docPath(DocID docid) const;

  void loadDocument(RefPtr<Document> doc);
  void commitDocument(RefPtr<Document> doc);

  std::mutex update_lock_;
  String path_;
};

} // namespace cm

#endif
