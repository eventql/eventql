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
#include "zbase/mapreduce/MapReduceTask.h"
#include "zbase/core/TSDBService.h"
#include "zbase/AnalyticsAuth.h"
#include "zbase/CustomerConfig.h"
#include "zbase/ConfigDirectory.h"

using namespace stx;

namespace zbase {

class MapReduceTaskBuilder {
public:

  MapReduceTaskBuilder(
      const AnalyticsSession& session,
      RefPtr<MapReduceJobSpec> job_spec,
      AnalyticsAuth* auth,
      zbase::PartitionMap* pmap,
      zbase::ReplicationScheme* repl,
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

  AnalyticsSession session_;
  RefPtr<MapReduceJobSpec> job_spec_;
  AnalyticsAuth* auth_;
  zbase::PartitionMap* pmap_;
  zbase::ReplicationScheme* repl_;
  TSDBService* tsdb_;
  String cachedir_;
};

} // namespace zbase

