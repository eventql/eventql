/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_INDEXWRITER_H
#define _CM_INDEXWRITER_H
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
#include "fnord-base/thread/taskscheduler.h"
#include <fnord-fts/fts.h>
#include <fnord-fts/fts_common.h>
#include "fnord-mdb/MDB.h"
#include "fnord-base/stats/stats.h"
#include "FeatureIndex.h"
#include "DocStore.h"
#include "IndexRequest.h"
#include "FeatureIndexWriter.h"
#include "ItemRef.h"

using namespace fnord;

namespace cm {

class IndexWriter : public RefCounted {
public:

  static RefPtr<IndexWriter> openIndex(
      const String& index_path,
      const String& conf_path);

  ~IndexWriter();

  void updateDocument(const IndexRequest& index_request);
  void commit();

  void rebuildFTS();
  void rebuildFTS(DocID doc);
  void rebuildFTS(RefPtr<Document> doc);

  RefPtr<mdb::MDBTransaction> dbTransaction();

  void exportStats(const String& prefix);

protected:

  IndexWriter(
      FeatureSchema schema,
      RefPtr<mdb::MDB> db,
      std::shared_ptr<fts::IndexWriter> fts);

  FeatureSchema schema_;
  RefPtr<mdb::MDB> db_;
  RefPtr<mdb::MDBTransaction> db_txn_;
  RefPtr<FeatureIndexWriter> feature_idx_;
  std::shared_ptr<fts::IndexWriter> fts_;

  fnord::stats::Counter<uint64_t> stat_documents_indexed_total_;
  fnord::stats::Counter<uint64_t> stat_documents_indexed_success_;
  fnord::stats::Counter<uint64_t> stat_documents_indexed_error_;
  fnord::stats::Counter<uint64_t> stat_documents_indexed_fts_;
};

} // namespace cm

#endif
