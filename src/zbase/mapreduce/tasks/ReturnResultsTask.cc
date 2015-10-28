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
#include "sstable/sstablereader.h"

using namespace stx;

namespace zbase {

ReturnResultsTask::ReturnResultsTask(
    Vector<RefPtr<MapReduceTask>> sources,
    MapReduceShardList* shards) :
    sources_(sources) {
  Vector<size_t> input;

  for (const auto& src : sources_) {
    auto src_indexes = src->shards();
    input.insert(input.end(), src_indexes.begin(), src_indexes.end());
  }

  auto shard = mkRef(new MapReduceTaskShard());
  shard->task = this;
  shard->dependencies = input;

  addShard(shard, shards);
}

Option<MapReduceShardResult> ReturnResultsTask::execute(
    RefPtr<MapReduceTaskShard> shard,
    RefPtr<MapReduceScheduler> job) {
  for (const auto& input : shard->dependencies) {
    job->downloadResult(
        input,
        [&job] (
            const void* key,
            size_t key_len,
            const void* val,
            size_t val_len) {
      job->sendResult(
          String((const char*) key, key_len),
          String((const char*) val, val_len));
    });
  }

  return None<MapReduceShardResult>();
}

} // namespace zbase

