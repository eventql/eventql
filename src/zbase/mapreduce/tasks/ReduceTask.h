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
      const String& method_name,
      Vector<RefPtr<MapReduceTask>> sources,
      size_t num_shards,
      AnalyticsAuth* auth);

  Vector<size_t> build(MapReduceShardList* shards) override;

  Option<MapReduceShardResult> execute(
      RefPtr<MapReduceTaskShard> shard,
      RefPtr<MapReduceScheduler> job) override;

protected:
  AnalyticsSession session_;
  RefPtr<MapReduceJobSpec> job_spec_;
  String method_name_;
  Vector<RefPtr<MapReduceTask>> sources_;
  size_t num_shards_;
  AnalyticsAuth* auth_;
};

} // namespace zbase


