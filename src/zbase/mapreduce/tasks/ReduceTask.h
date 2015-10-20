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

class ReduceTask : public MapReduceTask {
public:

  ReduceTask(
      const AnalyticsSession& session,
      RefPtr<MapReduceJobSpec> job_spec,
      Vector<RefPtr<MapReduceTask>> sources,
      AnalyticsAuth* auth);

  Vector<size_t> build(MapReduceShardList* shards) override;

  MapReduceShardResult execute(
      RefPtr<MapReduceTaskShard> shard,
      MapReduceScheduler* job) override;

protected:
  AnalyticsSession session_;
  RefPtr<MapReduceJobSpec> job_spec_;
  Vector<RefPtr<MapReduceTask>> sources_;
  AnalyticsAuth* auth_;
};

} // namespace zbase


