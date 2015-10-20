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
#include "stx/thread/ThreadPool.h"
#include "zbase/mapreduce/MapReduceTask.h"

using namespace stx;

namespace zbase {

class MapReduceScheduler {
public:
  static const size_t kDefaultMaxConcurrentTasks = 16;

  MapReduceScheduler(
      const MapReduceShardList& shards,
      thread::ThreadPool* tpool,
      size_t max_concurrent_tasks = kDefaultMaxConcurrentTasks);

  void execute();

protected:

  enum class MapReduceShardStatus { PENDING, RUNNING, COMPLETED, ERROR };

  void startJobs();

  MapReduceShardList shards_;
  Vector<MapReduceShardStatus> shard_status_;
  thread::ThreadPool* tpool_;

  size_t max_concurrent_tasks_;
  bool done_;
  size_t num_shards_running_;
  size_t num_shards_completed_;

  std::mutex mutex_;
  std::condition_variable cv_;
};

} // namespace zbase

