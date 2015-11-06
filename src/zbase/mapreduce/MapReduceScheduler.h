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
#include "zbase/core/TSDBService.h"
#include "zbase/AnalyticsAuth.h"

using namespace stx;

namespace zbase {

class MapReduceScheduler : public RefCounted {
public:
  static const size_t kDefaultMaxConcurrentTasks = 32;

  MapReduceScheduler(
      const AnalyticsSession& session,
      RefPtr<MapReduceJobSpec> job,
      const MapReduceShardList& shards,
      thread::ThreadPool* tpool,
      AnalyticsAuth* auth,
      const String& cachedir,
      size_t max_concurrent_tasks = kDefaultMaxConcurrentTasks);

  void execute();

  void sendResult(const String& key, const String& value);

  Option<String> getResultURL(size_t task_index);
  Option<SHA1Hash> getResultID(size_t task_index);
  Option<ReplicaRef> getResultHost(size_t task_index);

  void downloadResult(
      size_t task_index,
      Function<void (const void*, size_t, const void*, size_t)> fn);

protected:

  enum class MapReduceShardStatus { PENDING, RUNNING, COMPLETED, ERROR };

  size_t startJobs();

  AnalyticsSession session_;
  RefPtr<MapReduceJobSpec> job_;
  MapReduceShardList shards_;
  Vector<MapReduceShardStatus> shard_status_;
  Vector<Option<MapReduceShardResult>> shard_results_;
  Vector<size_t> shard_perms_;
  thread::ThreadPool* tpool_;
  AnalyticsAuth* auth_;
  String cachedir_;

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

