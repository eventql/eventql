/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include "stx/stdtypes.h"
#include "zbase/mapreduce/MapReduceTask.h"
#include "zbase/core/TSDBService.h"
#include "zbase/AnalyticsAuth.h"

using namespace stx;

namespace zbase {

class MapTableTask : public MapReduceTask {
public:

  MapTableTask(
      const AnalyticsSession& session,
      const TSDBTableRef& table,
      AnalyticsAuth* auth,
      zbase::PartitionMap* pmap,
      zbase::ReplicationScheme* repl);

  Vector<size_t> build(MapReduceShardList* shards) override;

  MapReduceShardResult execute(
      const MapReduceTaskShard& shard,
      MapReduceScheduler* job) override;

protected:
  AnalyticsSession session_;
  TSDBTableRef table_ref_;
  AnalyticsAuth* auth_;
  zbase::PartitionMap* pmap_;
  zbase::ReplicationScheme* repl_;
};

} // namespace zbase

