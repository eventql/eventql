/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_INDEXBUILD_H
#define _CM_INDEXBUILD_H
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
#include "FullIndex.h"
#include "IndexRequest.h"
#include "FeatureIndexWriter.h"
#include "ItemRef.h"

using namespace fnord;

namespace cm {

class IndexBuild {
public:

  IndexBuild(
      FeatureIndexWriter* feature_idx,
      FullIndex* full_idx);

  void updateDocument(const IndexRequest& index_request);
  void commit();

  void rebuildFTS();

protected:
  FeatureIndexWriter* feature_idx_;
  FullIndex* full_idx_;
};

} // namespace cm

#endif
