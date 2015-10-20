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
    AnalyticsAuth* auth,
    zbase::PartitionMap* pmap,
    zbase::ReplicationScheme* repl) :
    auth_(auth),
    pmap_(pmap),
    repl_(repl) {}

Vector<size_t> MapTableTask::build(MapReduceShardList* shards) {

  Vector<SHA1Hash> partitions;

  Vector<size_t> indexes;
  for (const auto& partition : partitions) {
    MapReduceTaskShard shard;
    shard.task = this;

    indexes.emplace_back(shards->size());
    shards->emplace_back(shard);
  }

  return indexes;
}

} // namespace zbase

