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
#include "eventql/util/stdtypes.h"
#include "brokerd/RemoteFeed.h"
#include "brokerd/RemoteFeedWriter.h"
#include "eventql/util/thread/taskscheduler.h"
#include <eventql/util/fts.h>
#include <eventql/util/fts_common.h>
#include "eventql/util/mdb/MDB.h"
#include "eventql/util/stats/stats.h"
#include "DocStore.h"
#include "IndexChangeRequest.h"
#include "DocIndex.h"
#include <eventql/docdb/ItemRef.h>

using namespace stx;

namespace zbase {

class IndexWriter : public RefCounted {
public:

  static RefPtr<IndexWriter> openIndex(
      const String& index_path,
      const String& conf_path);

  ~IndexWriter();

  void updateDocument(const IndexChangeRequest& index_request);
  void commit();

  void rebuildFTS(size_t commit_size = 8192);
  void rebuildFTS(DocID doc);
  void rebuildFTS(RefPtr<Document> doc);

  RefPtr<mdb::MDBTransaction> dbTransaction();

  void exportStats(const String& prefix);

protected:

  IndexWriter(
      RefPtr<DocIndex> doc_idx,
      std::shared_ptr<fts::IndexWriter> fts_idx);

  FeatureSchema schema_;
  RefPtr<DocIndex> doc_idx_;
  std::shared_ptr<fts::IndexWriter> fts_idx_;

  stx::stats::Counter<uint64_t> stat_documents_indexed_total_;
  stx::stats::Counter<uint64_t> stat_documents_indexed_success_;
  stx::stats::Counter<uint64_t> stat_documents_indexed_error_;
  stx::stats::Counter<uint64_t> stat_documents_indexed_fts_;
};

} // namespace zbase

#endif
