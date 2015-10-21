/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "zbase/mapreduce/tasks/ReduceTask.h"

using namespace stx;

namespace zbase {

ReduceTask::ReduceTask(
    const AnalyticsSession& session,
    RefPtr<MapReduceJobSpec> job_spec,
    const String& method_name,
    Vector<RefPtr<MapReduceTask>> sources,
    size_t num_shards,
    AnalyticsAuth* auth) :
    session_(session),
    job_spec_(job_spec),
    method_name_(method_name),
    sources_(sources),
    num_shards_(num_shards),
    auth_(auth) {}

Vector<size_t> ReduceTask::build(MapReduceShardList* shards) {
  Vector<size_t> out_indexes;
  Vector<size_t> in_indexes;

  for (const auto& src : sources_) {
    auto src_indexes = src->build(shards);
    in_indexes.insert(in_indexes.end(), src_indexes.begin(), src_indexes.end());
  }

  for (size_t shard_idx = 0; shard_idx < num_shards_; shard_idx++) {
    auto shard = mkRef(new MapReduceTaskShard());
    shard->task = this;
    shard->dependencies = in_indexes;

    out_indexes.emplace_back(shards->size());
    shards->emplace_back(shard);
  }

  return out_indexes;
}

Option<MapReduceShardResult> ReduceTask::execute(
    RefPtr<MapReduceTaskShard> shard,
    RefPtr<MapReduceScheduler> job) {
  iputs("execute reduce", 1);
  return None<MapReduceShardResult>();
}

} // namespace zbase

