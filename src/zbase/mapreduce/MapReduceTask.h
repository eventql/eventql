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
#include "stx/autoref.h"
#include "stx/option.h"
#include "stx/json/json.h"
#include "zbase/core/ReplicationScheme.h"

using namespace stx;

namespace zbase {

class MapReduceTask;

struct MapReduceTaskShard : public RefCounted {
  RefPtr<MapReduceTask> task;
  Vector<size_t> dependencies;
};

using MapReduceShardList = Vector<RefPtr<MapReduceTaskShard>>;

struct MapReduceShardResult {
  ReplicaRef host;
  SHA1Hash result_id;
};

struct MapReduceJobStatus {
  size_t num_tasks_total;
  size_t num_tasks_completed;
  size_t num_tasks_running;
};

class MapReduceJobSpec : public RefCounted {
public:

  void onProgress(Function<void (const MapReduceJobStatus& status)> fn);
  void updateProgress(const MapReduceJobStatus& status);

  void onResult(Function<void (const String& key, const String& value)> fn);
  void sendResult(const String& key, const String& value);

  String program_source;
protected:
  Function<void (const MapReduceJobStatus& status)> on_progress_;
  Function<void (const String& key, const String& value)> on_result_;
};

class MapReduceScheduler;

class MapReduceTask : public RefCounted {
public:

  virtual Option<MapReduceShardResult> execute(
      RefPtr<MapReduceTaskShard> shard,
      RefPtr<MapReduceScheduler> job) = 0;

  Vector<size_t> shards() const;

protected:

  size_t addShard(
      RefPtr<MapReduceTaskShard> shard,
      MapReduceShardList* shards);

  Vector<size_t> shards_;
};


} // namespace zbase

