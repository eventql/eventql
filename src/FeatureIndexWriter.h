/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_FEATUREINDEXWRITER_H
#define _CM_FEATUREINDEXWRITER_H
#include <mutex>
#include <stdlib.h>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include <queue>
#include "fnord-base/stdtypes.h"
#include "fnord-feeds/RemoteFeed.h"
#include "fnord-feeds/RemoteFeedWriter.h"
#include "fnord-rpc/RPC.h"
#include "fnord-rpc/RPCClient.h"
#include "fnord-base/thread/taskscheduler.h"
#include "fnord-mdb/MDB.h"
#include "fnord-base/stats/stats.h"
#include "FeatureIndex.h"
#include "IndexChangeRequest.h"
#include "ItemRef.h"
#include "Document.h"
#include "logjoin/TrackedSession.h"
#include "logjoin/TrackedQuery.h"
#include "logjoin/LogJoinShard.h"

using namespace fnord;

namespace cm {
class CustomerNamespace;

class FeatureIndexWriter : public RefCounted {
public:

  FeatureIndexWriter(
      const String& db_path,
      const String& index_name,
      bool readonly);

  ~FeatureIndexWriter();

  RefPtr<Document> findDocument(const DocID& docid);
  void listDocuments(Function<bool (const DocID& id)> fn);

  Option<String> getField(const DocID& docid, const String& feature);

  //void updateDocument(const IndexChangeRequest& index_request);

  void updateDocument(
      const DocID& docid,
      const Vector<Pair<String, String>>& fields);

  void commit(bool sync = false);

protected:

  HashMap<String, uint32_t> schema_;
  bool readonly_;
  RefPtr<mdb::MDB> db_;
  RefPtr<mdb::MDBTransaction> txn_;
};

} // namespace cm

#endif
