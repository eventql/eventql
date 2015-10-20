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
    Vector<RefPtr<MapReduceTask>> sources) :
    sources_(sources) {}

Vector<size_t> ReduceTask::build(MapReduceShardList* shards) {
  Vector<size_t> out_indexes;
  Vector<size_t> in_indexes;

  for (const auto& src : sources_) {
    auto src_indexes = src->build(shards);
    in_indexes.insert(in_indexes.end(), src_indexes.begin(), src_indexes.end());
  }

  size_t nshards = 1;
  for (size_t shard_idx = 0; shard_idx < nshards; shard_idx++) {
    MapReduceTaskShard shard;
    shard.task = this;
    shard.dependencies = in_indexes;

    out_indexes.emplace_back(shards->size());
    shards->emplace_back(shard);
  }

  return out_indexes;
}

} // namespace zbase

