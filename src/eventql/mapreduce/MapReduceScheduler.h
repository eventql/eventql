/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#pragma once
#include "eventql/util/stdtypes.h"
#include "eventql/util/thread/threadpool.h"
#include "eventql/mapreduce/MapReduceTask.h"
#include "eventql/core/TSDBService.h"
#include "eventql/AnalyticsAuth.h"

using namespace stx;

namespace zbase {

class MapReduceScheduler : public RefCounted {
public:
  static const size_t kDefaultMaxConcurrentTasks = 32;

  MapReduceScheduler(
      const AnalyticsSession& session,
      RefPtr<MapReduceJobSpec> job,
      thread::ThreadPool* tpool,
      AnalyticsAuth* auth,
      const String& cachedir,
      size_t max_concurrent_tasks = kDefaultMaxConcurrentTasks);

  void execute(const MapReduceShardList& shards);

  void sendResult(const String& value);
  void sendLogline(const String& logline);

  Option<String> getResultURL(size_t task_index);
  Option<SHA1Hash> getResultID(size_t task_index);
  Option<ReplicaRef> getResultHost(size_t task_index);

  void downloadResult(
      size_t task_index,
      Function<void (const void*, size_t, const void*, size_t)> fn);

  RefPtr<MapReduceJobSpec> jobSpec();

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

