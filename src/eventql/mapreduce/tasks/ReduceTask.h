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
#include "eventql/eventql.h"
#include "eventql/server/session.h"
#include "eventql/util/stdtypes.h"
#include "eventql/mapreduce/MapReduceTask.h"
#include "eventql/db/TSDBService.h"
#include "eventql/AnalyticsAuth.h"

namespace eventql {

struct ReduceTaskShard : public MapReduceTaskShard {
  size_t shard;
};

class ReduceTask : public MapReduceTask {
public:

  ReduceTask(
      Session* session,
      const String& reduce_fn,
      const String& globals,
      const String& params,
      Vector<RefPtr<MapReduceTask>> sources,
      size_t num_shards,
      MapReduceShardList* shards,
      AnalyticsAuth* auth,
      eventql::ReplicationScheme* repl);

  Option<MapReduceShardResult> execute(
      RefPtr<MapReduceTaskShard> shard,
      RefPtr<MapReduceScheduler> job) override;

protected:

  Option<MapReduceShardResult> executeRemote(
      RefPtr<MapReduceTaskShard> shard,
      RefPtr<MapReduceScheduler> job,
      const Vector<String>& input_tables,
      const ReplicaRef& host);

  Session* session_;
  String reduce_fn_;
  String globals_;
  String params_;
  Vector<RefPtr<MapReduceTask>> sources_;
  size_t num_shards_;
  AnalyticsAuth* auth_;
  eventql::ReplicationScheme* repl_;
};

} // namespace eventql


