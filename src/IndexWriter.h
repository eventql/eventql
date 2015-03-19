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

  static RefPtr<IndexWriter> openIndex(const String& path);

  ~IndexWriter();

  void updateDocument(const IndexRequest& index_request);
  void commit();

  void rebuildFTS();

  RefPtr<mdb::MDB> featureDB();

protected:

  IndexWriter(
      FeatureSchema schema,
      RefPtr<mdb::MDB> db,
      RefPtr<DocStore> docs,
      std::shared_ptr<fts::IndexWriter> fts);

  void updateDocumentFTS(RefPtr<Document> doc);

  FeatureSchema schema_;
  RefPtr<mdb::MDB> db_;
  RefPtr<DocStore> docs_;
  RefPtr<FeatureIndexWriter> feature_idx_;
  std::shared_ptr<fts::IndexWriter> fts_;
};

} // namespace cm

#endif
