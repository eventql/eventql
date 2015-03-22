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
#include "IndexRequest.h"
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

  FeatureIndexWriter(const FeatureSchema* schema);

  void updateDocument(
      const IndexRequest& index_request,
      mdb::MDBTransaction* featuredb_txn);

  RefPtr<Document> findDocument(
      const DocID& docid,
      mdb::MDBTransaction* featuredb_txn);

  void listDocuments(
      Function<bool (const DocID& id)> fn,
      mdb::MDBTransaction* featuredb_txn);

protected:

  void updateFeatureIndex(
      const IndexRequest& index_request,
      mdb::MDBTransaction* featuredb_txn);

  void updateFeatureIndex(
      const DocID& docid,
      const Vector<Pair<FeatureID, String>>& features,
      mdb::MDBTransaction* featuredb_txn);

  const FeatureSchema* feature_schema_;

};
} // namespace cm

#endif
