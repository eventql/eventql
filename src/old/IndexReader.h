/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_INDEXREADER_H
#define _CM_INDEXREADER_H
#include <mutex>
#include <stdlib.h>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include <queue>
#include "stx/stdtypes.h"
#include "stx/thread/taskscheduler.h"
#include <fnord-fts/fts.h>
#include <fnord-fts/fts_common.h>
#include "stx/mdb/MDB.h"
#include "stx/stats/stats.h"
#include "FeatureIndex.h"
#include "DocStore.h"
#include "IndexChangeRequest.h"
#include "DocIndex.h"
#include <zbase/docdb/ItemRef.h>

using namespace stx;

namespace zbase {

class IndexReader : public RefCounted {
public:

  static RefPtr<IndexReader> openIndex(const String& path);

  ~IndexReader();

  RefPtr<DocIndex> docIndex();
  std::shared_ptr<fts::IndexSearcher> ftsSearcher();

protected:

  IndexReader(
      RefPtr<DocIndex> doc_idx,
      std::shared_ptr<fts::IndexReader> fts);

  RefPtr<DocIndex> doc_idx_;
  std::shared_ptr<fts::IndexReader> fts_;
  std::shared_ptr<fts::IndexSearcher> fts_searcher_;
};

} // namespace zbase

#endif
