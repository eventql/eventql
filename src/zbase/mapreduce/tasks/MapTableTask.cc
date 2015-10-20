/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "stx/SHA1.h"
#include "zbase/mapreduce/tasks/MapTableTask.h"

using namespace stx;

namespace zbase {

MapTableTask::MapTableTask(
    const AnalyticsSession& session,
    RefPtr<MapReduceJobSpec> job_spec,
    const TSDBTableRef& table_ref,
    AnalyticsAuth* auth,
    zbase::PartitionMap* pmap,
    zbase::ReplicationScheme* repl) :
    session_(session),
    job_spec_(job_spec),
    table_ref_(table_ref),
    auth_(auth),
    pmap_(pmap),
    repl_(repl) {}

Vector<size_t> MapTableTask::build(MapReduceShardList* shards) {
  auto table = pmap_->findTable(session_.customer(), table_ref_.table_key);
  if (table.isEmpty()) {
    RAISEF(kNotFoundError, "table not found: $0", table_ref_.table_key);
  }

  auto partitioner = table.get()->partitioner();
  auto partitions = partitioner->partitionKeysFor(table_ref_);

  Vector<size_t> indexes;
  for (const auto& partition : partitions) {
    MapReduceTaskShard shard;
    shard.task = this;

    indexes.emplace_back(shards->size());
    shards->emplace_back(shard);
  }

  return indexes;
}

MapReduceShardResult MapTableTask::execute(
    const MapReduceTaskShard& shard,
    MapReduceScheduler* job) {
  iputs("execute map...", 1);
}

} // namespace zbase

