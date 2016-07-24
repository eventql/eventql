/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#include "eventql/util/autoref.h"
#include "eventql/util/option.h"
#include "eventql/util/SHA1.h"
#include "eventql/util/json/json.h"
#include "eventql/eventql.h"

namespace eventql {

class MapReduceTask;

struct MapReduceTaskShard : public RefCounted {
  RefPtr<MapReduceTask> task;
  Vector<size_t> dependencies;
};

using MapReduceShardList = Vector<RefPtr<MapReduceTaskShard>>;

struct MapReduceShardResult {
  String server_id;
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

  void onResult(Function<void (const String& value)> fn);
  void sendResult(const String& value);

  void onLogline(Function<void (const String& line)> fn);
  void sendLogline(const String& line);

protected:
  Function<void (const MapReduceJobStatus& status)> on_progress_;
  Function<void (const String& value)> on_result_;
  Function<void (const String& line)> on_logline_;
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


} // namespace eventql

