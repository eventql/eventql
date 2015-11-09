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
#include "zbase/AnalyticsSession.pb.h"

using namespace stx;

namespace zbase {

class ReturnResultsTask : public MapReduceTask {
public:

  ReturnResultsTask(
      Vector<RefPtr<MapReduceTask>> sources,
      MapReduceShardList* shards,
      const AnalyticsSession& session,
      const String& serialize_fn,
      const String& globals,
      const String& params);

  Option<MapReduceShardResult> execute(
      RefPtr<MapReduceTaskShard> shard,
      RefPtr<MapReduceScheduler> job) override;

protected:
  Vector<RefPtr<MapReduceTask>> sources_;
  AnalyticsSession session_;
  const String& serialize_fn_;
  const String& globals_;
  const String& params_;
};

} // namespace zbase


