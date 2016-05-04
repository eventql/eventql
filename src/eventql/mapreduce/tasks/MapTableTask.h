/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include "eventql/util/stdtypes.h"
#include "eventql/mapreduce/MapReduceTask.h"
#include "eventql/core/TSDBService.h"
#include "eventql/AnalyticsAuth.h"

using namespace stx;

namespace zbase {

struct MapTableTaskShard : public MapReduceTaskShard {
  TSDBTableRef table_ref;
};

class MapTableTask : public MapReduceTask {
public:

  MapTableTask(
      const AnalyticsSession& session,
      const TSDBTableRef& table,
      const String& map_function,
      const String& globals,
      const String& params,
      MapReduceShardList* shards,
      AnalyticsAuth* auth,
      zbase::PartitionMap* pmap,
      zbase::ReplicationScheme* repl);

  Option<MapReduceShardResult> execute(
      RefPtr<MapReduceTaskShard> shard,
      RefPtr<MapReduceScheduler> job) override;

  void setRequiredColumns(const Set<String>& columns);

protected:

  Option<MapReduceShardResult> executeRemote(
      RefPtr<MapTableTaskShard> shard,
      RefPtr<MapReduceScheduler> job,
      const ReplicaRef& host);

  AnalyticsSession session_;
  TSDBTableRef table_ref_;
  String map_function_;
  String globals_;
  String params_;
  AnalyticsAuth* auth_;
  zbase::PartitionMap* pmap_;
  zbase::ReplicationScheme* repl_;
  Set<String> required_columns_;
};

} // namespace zbase

