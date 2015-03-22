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
#include "fnord-base/stdtypes.h"
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

class IndexReader : public RefCounted {
public:

  static RefPtr<IndexReader> openIndex(const String& path);

  ~IndexReader();

  RefPtr<FeatureIndexWriter> docIndex();
  std::shared_ptr<fts::IndexSearcher> ftsSearcher();

  RefPtr<mdb::MDB> db_;
protected:

  IndexReader(
      FeatureSchema schema,
      RefPtr<mdb::MDB> db,
      std::shared_ptr<fts::IndexReader> fts);

  FeatureSchema schema_;
  RefPtr<FeatureIndexWriter> doc_idx_;
  std::shared_ptr<fts::IndexReader> fts_;
  std::shared_ptr<fts::IndexSearcher> fts_searcher_;
};

} // namespace cm

#endif
