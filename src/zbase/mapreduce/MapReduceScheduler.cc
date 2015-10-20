/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "stx/logging.h"
#include "zbase/mapreduce/MapReduceScheduler.h"

using namespace stx;

namespace zbase {

MapReduceScheduler::MapReduceScheduler(
    const MapReduceShardList& shards,
    thread::ThreadPool* tpool,
    size_t max_concurrent_tasks /* = kDefaultMaxConcurrentTasks */) :
    shards_(shards),
    shard_status_(shards_.size(), MapReduceShardStatus::PENDING),
    tpool_(tpool),
    max_concurrent_tasks_(max_concurrent_tasks),
    done_(false),
    num_shards_running_(0),
    num_shards_completed_(0) {}

void MapReduceScheduler::execute() {
  startJobs();

  std::unique_lock<std::mutex> lk(mutex_);
  while (!done_) {
    logDebug(
        "z1.mapreduce",
        "Running job; progress=$0/$1",
        num_shards_completed_,
        shards_.size());

    cv_.wait(lk);
  }
}

void MapReduceScheduler::startJobs() {
  std::unique_lock<std::mutex> lk(mutex_);

  if (num_shards_running_ >= max_concurrent_tasks_) {
    return;
  }

  if (num_shards_completed_ + num_shards_running_ >= shards_.size()) {
    return;
  }

  for (size_t i = 0; i < shards_.size(); ++i) {
    if (shard_status_[i] != MapReduceShardStatus::PENDING) {
      continue;
    }

    iputs("start task $0", i);
  }
}

} // namespace zbase

