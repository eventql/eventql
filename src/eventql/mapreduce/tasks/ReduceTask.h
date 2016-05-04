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
#include "eventql/mapreduce/MapReduceTask.h"
#include "eventql/core/TSDBService.h"
#include "eventql/AnalyticsAuth.h"

using namespace stx;

namespace zbase {

struct ReduceTaskShard : public MapReduceTaskShard {
  size_t shard;
};

class ReduceTask : public MapReduceTask {
public:

  ReduceTask(
      const AnalyticsSession& session,
      const String& reduce_fn,
      const String& globals,
      const String& params,
      Vector<RefPtr<MapReduceTask>> sources,
      size_t num_shards,
      MapReduceShardList* shards,
      AnalyticsAuth* auth,
      zbase::ReplicationScheme* repl);

  Option<MapReduceShardResult> execute(
      RefPtr<MapReduceTaskShard> shard,
      RefPtr<MapReduceScheduler> job) override;

protected:

  Option<MapReduceShardResult> executeRemote(
      RefPtr<MapReduceTaskShard> shard,
      RefPtr<MapReduceScheduler> job,
      const Vector<String>& input_tables,
      const ReplicaRef& host);

  AnalyticsSession session_;
  String reduce_fn_;
  String globals_;
  String params_;
  Vector<RefPtr<MapReduceTask>> sources_;
  size_t num_shards_;
  AnalyticsAuth* auth_;
  zbase::ReplicationScheme* repl_;
};

} // namespace zbase


