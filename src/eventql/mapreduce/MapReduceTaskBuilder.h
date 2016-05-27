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
#include "eventql/auth/internal_auth.h"
#include "eventql/config/namespace_config.h"
#include "eventql/config/config_directory.h"

namespace eventql {

class MapReduceTaskBuilder : public RefCounted {
public:

  MapReduceTaskBuilder(
      Session* session,
      InternalAuth* auth,
      eventql::PartitionMap* pmap,
      eventql::ReplicationScheme* repl,
      TSDBService* tsdb,
      const String& cachedir);

  MapReduceShardList fromJSON(
      const json::JSONObject::const_iterator& begin,
      const json::JSONObject::const_iterator& end);

protected:

  RefPtr<MapReduceTask> getJob(
      const String& job_name,
      MapReduceShardList* shards,
      HashMap<String, json::JSONObject>* job_definitions,
      HashMap<String, RefPtr<MapReduceTask>>* jobs);

  RefPtr<MapReduceTask> buildMapTableTask(
      const json::JSONObject& job,
      MapReduceShardList* shards,
      HashMap<String, json::JSONObject>* job_definitions,
      HashMap<String, RefPtr<MapReduceTask>>* jobs);

  RefPtr<MapReduceTask> buildReduceTask(
      const json::JSONObject& job,
      MapReduceShardList* shards,
      HashMap<String, json::JSONObject>* job_definitions,
      HashMap<String, RefPtr<MapReduceTask>>* jobs);

  RefPtr<MapReduceTask> buildReturnResultsTask(
      const json::JSONObject& job,
      MapReduceShardList* shards,
      HashMap<String, json::JSONObject>* job_definitions,
      HashMap<String, RefPtr<MapReduceTask>>* jobs);

  RefPtr<MapReduceTask> buildSaveToTableTask(
      const json::JSONObject& job,
      MapReduceShardList* shards,
      HashMap<String, json::JSONObject>* job_definitions,
      HashMap<String, RefPtr<MapReduceTask>>* jobs);

  RefPtr<MapReduceTask> buildSaveToTablePartitionTask(
      const json::JSONObject& job,
      MapReduceShardList* shards,
      HashMap<String, json::JSONObject>* job_definitions,
      HashMap<String, RefPtr<MapReduceTask>>* jobs);

  Session* session_;
  InternalAuth* auth_;
  eventql::PartitionMap* pmap_;
  eventql::ReplicationScheme* repl_;
  TSDBService* tsdb_;
  String cachedir_;
};

} // namespace eventql

