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

struct MapTableTaskShard : public MapReduceTaskShard {
  TSDBTableRef table_ref;
};

class MapTableTask : public MapReduceTask {
public:

  MapTableTask(
      const AnalyticsSession& session,
      RefPtr<MapReduceJobSpec> job_spec,
      const TSDBTableRef& table,
      const String& method_name,
      AnalyticsAuth* auth,
      zbase::PartitionMap* pmap,
      zbase::ReplicationScheme* repl);

  Vector<size_t> build(MapReduceShardList* shards) override;

  MapReduceShardResult execute(
      RefPtr<MapReduceTaskShard> shard,
      RefPtr<MapReduceScheduler> job) override;

protected:

  MapReduceShardResult executeRemote(
      RefPtr<MapTableTaskShard> shard,
      RefPtr<MapReduceScheduler> job,
      const ReplicaRef& host);

  AnalyticsSession session_;
  RefPtr<MapReduceJobSpec> job_spec_;
  TSDBTableRef table_ref_;
  String method_name_;
  AnalyticsAuth* auth_;
  zbase::PartitionMap* pmap_;
  zbase::ReplicationScheme* repl_;
};

} // namespace zbase

