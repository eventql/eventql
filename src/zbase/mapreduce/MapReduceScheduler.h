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
#include "stx/thread/threadpool.h"
#include "zbase/mapreduce/MapReduceTask.h"

using namespace stx;

namespace zbase {

class MapReduceScheduler : public RefCounted {
public:
  static const size_t kDefaultMaxConcurrentTasks = 16;

  MapReduceScheduler(
      RefPtr<MapReduceJobSpec> job,
      const MapReduceShardList& shards,
      thread::ThreadPool* tpool,
      size_t max_concurrent_tasks = kDefaultMaxConcurrentTasks);

  void execute();

protected:

  enum class MapReduceShardStatus { PENDING, RUNNING, COMPLETED, ERROR };

  size_t startJobs();

  RefPtr<MapReduceJobSpec> job_;
  MapReduceShardList shards_;
  Vector<MapReduceShardStatus> shard_status_;
  Vector<Option<MapReduceShardResult>> shard_results_;
  thread::ThreadPool* tpool_;

  size_t max_concurrent_tasks_;
  bool done_;
  bool error_;
  size_t num_shards_running_;
  size_t num_shards_completed_;
  Vector<String> errors_;

  std::mutex mutex_;
  std::condition_variable cv_;
};

} // namespace zbase

