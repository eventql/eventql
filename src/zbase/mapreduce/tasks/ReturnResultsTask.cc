/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "zbase/mapreduce/tasks/ReturnResultsTask.h"
#include "zbase/mapreduce/MapReduceScheduler.h"

using namespace stx;

namespace zbase {

ReturnResultsTask::ReturnResultsTask(
    Vector<RefPtr<MapReduceTask>> sources) :
    sources_(sources) {}

Vector<size_t> ReturnResultsTask::build(MapReduceShardList* shards) {
  Vector<size_t> out_indexes;
  Vector<size_t> in_indexes;

  for (const auto& src : sources_) {
    auto src_indexes = src->build(shards);
    in_indexes.insert(in_indexes.end(), src_indexes.begin(), src_indexes.end());
  }

  auto shard = mkRef(new MapReduceTaskShard());
  shard->task = this;
  shard->dependencies = in_indexes;

  out_indexes.emplace_back(shards->size());
  shards->emplace_back(shard);

  return out_indexes;
}

Option<MapReduceShardResult> ReturnResultsTask::execute(
    RefPtr<MapReduceTaskShard> shard,
    RefPtr<MapReduceScheduler> job) {
  for (const auto& input : shard->dependencies) {
    auto input_sst = job->downloadResult(input);
    if (input_sst.isEmpty()) {
      continue;
    }

    job->sendResult("fixme", input_sst.get());
  }

  return None<MapReduceShardResult>();
}

} // namespace zbase

