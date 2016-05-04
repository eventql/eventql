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

class SaveToTablePartitionTask : public MapReduceTask {
public:

  SaveToTablePartitionTask(
      const AnalyticsSession& session,
      const String& table_name,
      const SHA1Hash& partition_key,
      Vector<RefPtr<MapReduceTask>> sources,
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
  String table_name_;
  SHA1Hash partition_key_;
  Vector<RefPtr<MapReduceTask>> sources_;
  AnalyticsAuth* auth_;
  zbase::ReplicationScheme* repl_;
};

} // namespace zbase


