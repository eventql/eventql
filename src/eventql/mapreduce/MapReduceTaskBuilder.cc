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
#include "eventql/mapreduce/MapReduceTaskBuilder.h"
#include "eventql/mapreduce/tasks/MapTableTask.h"
#include "eventql/mapreduce/tasks/ReduceTask.h"
#include "eventql/mapreduce/tasks/ReturnResultsTask.h"
#include "eventql/mapreduce/tasks/SaveToTableTask.h"
#include "eventql/mapreduce/tasks/SaveToTablePartitionTask.h"
#include "eventql/AnalyticsAuth.h"
#include "eventql/CustomerConfig.h"
#include "eventql/ConfigDirectory.h"

using namespace stx;

namespace eventql {

MapReduceTaskBuilder::MapReduceTaskBuilder(
    const AnalyticsSession& session,
    AnalyticsAuth* auth,
    eventql::PartitionMap* pmap,
    eventql::ReplicationScheme* repl,
    TSDBService* tsdb,
    const String& cachedir) :
    session_(session),
    auth_(auth),
    pmap_(pmap),
    repl_(repl),
    tsdb_(tsdb),
    cachedir_(cachedir) {}

MapReduceShardList MapReduceTaskBuilder::fromJSON(
    const json::JSONObject::const_iterator& begin,
    const json::JSONObject::const_iterator& end) {
  MapReduceShardList shards;
  HashMap<String, json::JSONObject> job_definitions;
  HashMap<String, RefPtr<MapReduceTask>> jobs;

  auto njobs = json::arrayLength(begin, end);
  for (size_t i = 0; i < njobs; ++i) {
    auto job = json::arrayLookup(begin, end, i); // O(N^2) but who cares...

    auto id = json::objectGetString(job, end, "id");
    if (id.isEmpty()) {
      RAISE(kRuntimeError, "illegal job definition: missing id field");
    }

    job_definitions.emplace(id.get(), json::JSONObject(job, job + job->size));
  }

  for (const auto& job : job_definitions) {
    getJob(job.first, &shards, &job_definitions, &jobs);
  }

  return shards;
}

RefPtr<MapReduceTask> MapReduceTaskBuilder::getJob(
    const String& job_name,
    MapReduceShardList* shards,
    HashMap<String, json::JSONObject>* job_definitions,
    HashMap<String, RefPtr<MapReduceTask>>* jobs) {
  const auto& job_iter = jobs->find(job_name);
  if (job_iter != jobs->end()) {
    return job_iter->second;
  }

  const auto& job_def_iter = job_definitions->find(job_name);
  if (job_def_iter == job_definitions->end()) {
    RAISEF(kNotFoundError, "job not found: $0", job_name);
  }

  const auto& job_def = job_def_iter->second;
  auto op = json::objectGetString(job_def, "op");
  if (op.isEmpty()) {
    RAISE(kRuntimeError, "illegal job definition: missing op field");
  }

  RefPtr<MapReduceTask> job;

  if (op.get() == "map_table") {
    job = buildMapTableTask(job_def, shards, job_definitions, jobs);
  }

  if (op.get() == "reduce") {
    job = buildReduceTask(job_def, shards, job_definitions, jobs);
  }

  if (op.get() == "return_results") {
    job = buildReturnResultsTask(job_def, shards, job_definitions, jobs);
  }

  if (op.get() == "save_to_table") {
    job = buildSaveToTableTask(job_def, shards, job_definitions, jobs);
  }

  if (op.get() == "save_to_table_partition") {
    job = buildSaveToTablePartitionTask(job_def, shards, job_definitions, jobs);
  }

  if (job.get() == nullptr) {
    RAISEF(kRuntimeError, "unknown operation: $0", op.get());
  }

  jobs->emplace(job_name, job);
  return job;
}

RefPtr<MapReduceTask> MapReduceTaskBuilder::buildMapTableTask(
    const json::JSONObject& job,
    MapReduceShardList* shards,
    HashMap<String, json::JSONObject>* job_definitions,
    HashMap<String, RefPtr<MapReduceTask>>* jobs) {
  TSDBTableRef table_ref;

  auto table_name = json::objectGetString(job, "table_name");
  if (table_name.isEmpty()) {
    RAISE(kRuntimeError, "missing field: table_name");
  } else {
    table_ref.table_key = table_name.get();
  }

  auto from = json::objectGetUInt64(job, "from");
  if (!from.isEmpty()) {
    table_ref.timerange_begin = UnixTime(from.get());
  }

  auto until = json::objectGetUInt64(job, "until");
  if (!until.isEmpty()) {
    table_ref.timerange_limit = UnixTime(until.get());
  }

  auto map_fn = json::objectGetString(job, "map_fn");
  if (map_fn.isEmpty()) {
    RAISE(kRuntimeError, "missing field: map_fn");
  }

  auto globals = json::objectGetString(job, "globals");
  if (globals.isEmpty()) {
    RAISE(kRuntimeError, "missing field: globals");
  }

  auto params = json::objectGetString(job, "params");
  if (params.isEmpty()) {
    RAISE(kRuntimeError, "missing field: params");
  }

  auto task = new MapTableTask(
      session_,
      table_ref,
      map_fn.get(),
      globals.get(),
      params.get(),
      shards,
      auth_,
      pmap_,
      repl_);

  auto required_columns = json::objectLookup(job, "required_columns");
  if (required_columns != job.end()) {
    Set<String> required_columns_set;

    auto ncols = json::arrayLength(required_columns, job.end());
    for (size_t i = 0; i < ncols; ++i) {
      auto jcol = json::arrayLookup(required_columns, job.end(), i); // O(N^2) but who cares...

      if (jcol->type != json::JSON_STRING) {
        RAISE(
            kRuntimeError,
            "required_columns parameter must be a list/array of strings");
      }

      required_columns_set.insert(jcol->data);
    }

    task->setRequiredColumns(required_columns_set);
  }

  return task;
}

RefPtr<MapReduceTask> MapReduceTaskBuilder::buildReduceTask(
    const json::JSONObject& job,
    MapReduceShardList* shards,
    HashMap<String, json::JSONObject>* job_definitions,
    HashMap<String, RefPtr<MapReduceTask>>* jobs) {
  auto src_begin = json::objectLookup(job, "sources");
  if (src_begin == job.end()) {
    RAISE(kRuntimeError, "missing field: sources");
  }

  auto num_shards = json::objectGetUInt64(job, "num_shards");
  if (num_shards.isEmpty()) {
    RAISE(kRuntimeError, "missing field: num_shards");
  }

  auto reduce_fn = json::objectGetString(job, "reduce_fn");
  if (reduce_fn.isEmpty()) {
    RAISE(kRuntimeError, "missing field: reduce_fn");
  }

  auto globals = json::objectGetString(job, "globals");
  if (globals.isEmpty()) {
    RAISE(kRuntimeError, "missing field: globals");
  }

  auto params = json::objectGetString(job, "params");
  if (params.isEmpty()) {
    RAISE(kRuntimeError, "missing field: params");
  }

  Vector<RefPtr<MapReduceTask>> sources;
  auto nsrc_begin = json::arrayLength(src_begin, job.end());
  for (size_t i = 0; i < nsrc_begin; ++i) {
    auto src_id = json::arrayGetString(src_begin, job.end(), i); // O(N^2) but who cares...
    if (src_id.isEmpty()) {
      RAISE(kRuntimeError, "illegal source definition");
    }

    sources.emplace_back(getJob(src_id.get(), shards, job_definitions, jobs));
  }

  return new ReduceTask(
      session_,
      reduce_fn.get(),
      globals.get(),
      params.get(),
      sources,
      num_shards.get(),
      shards,
      auth_,
      repl_);
}

RefPtr<MapReduceTask> MapReduceTaskBuilder::buildReturnResultsTask(
    const json::JSONObject& job,
    MapReduceShardList* shards,
    HashMap<String, json::JSONObject>* job_definitions,
    HashMap<String, RefPtr<MapReduceTask>>* jobs) {
  auto src_begin = json::objectLookup(job, "sources");
  if (src_begin == job.end()) {
    RAISE(kRuntimeError, "missing field: sources");
  }

  Vector<RefPtr<MapReduceTask>> sources;
  auto nsrc_begin = json::arrayLength(src_begin, job.end());
  for (size_t i = 0; i < nsrc_begin; ++i) {
    auto src_id = json::arrayGetString(src_begin, job.end(), i); // O(N^2) but who cares...
    if (src_id.isEmpty()) {
      RAISE(kRuntimeError, "illegal source definition");
    }

    sources.emplace_back(getJob(src_id.get(), shards, job_definitions, jobs));
  }

  auto serialize_fn = json::objectGetString(job, "serialize_fn");
  if (serialize_fn.isEmpty()) {
    RAISE(kRuntimeError, "missing field: serialize_fn");
  }

  auto globals = json::objectGetString(job, "globals");
  if (globals.isEmpty()) {
    RAISE(kRuntimeError, "missing field: globals");
  }

  auto params = json::objectGetString(job, "params");
  if (params.isEmpty()) {
    RAISE(kRuntimeError, "missing field: params");
  }

  return new ReturnResultsTask(
      sources,
      shards,
      session_,
      serialize_fn.get(),
      globals.get(),
      params.get());
}

RefPtr<MapReduceTask> MapReduceTaskBuilder::buildSaveToTableTask(
    const json::JSONObject& job,
    MapReduceShardList* shards,
    HashMap<String, json::JSONObject>* job_definitions,
    HashMap<String, RefPtr<MapReduceTask>>* jobs) {
  auto table_name = json::objectGetString(job, "table_name");
  if (table_name.isEmpty()) {
    RAISE(kRuntimeError, "missing field: table_name");
  }

  auto src_begin = json::objectLookup(job, "sources");
  if (src_begin == job.end()) {
    RAISE(kRuntimeError, "missing field: sources");
  }

  Vector<RefPtr<MapReduceTask>> sources;
  auto nsrc_begin = json::arrayLength(src_begin, job.end());
  for (size_t i = 0; i < nsrc_begin; ++i) {
    auto src_id = json::arrayGetString(src_begin, job.end(), i); // O(N^2) but who cares...
    if (src_id.isEmpty()) {
      RAISE(kRuntimeError, "illegal source definition");
    }

    sources.emplace_back(getJob(src_id.get(), shards, job_definitions, jobs));
  }

  return new SaveToTableTask(
      session_,
      table_name.get(),
      sources,
      shards,
      auth_,
      tsdb_);
}

RefPtr<MapReduceTask> MapReduceTaskBuilder::buildSaveToTablePartitionTask(
    const json::JSONObject& job,
    MapReduceShardList* shards,
    HashMap<String, json::JSONObject>* job_definitions,
    HashMap<String, RefPtr<MapReduceTask>>* jobs) {
  auto table_name = json::objectGetString(job, "table_name");
  if (table_name.isEmpty()) {
    RAISE(kRuntimeError, "missing field: table_name");
  }

  auto partition_key = json::objectGetString(job, "partition_key");
  if (partition_key.isEmpty()) {
    RAISE(kRuntimeError, "missing field: partition_key");
  }

  auto src_begin = json::objectLookup(job, "sources");
  if (src_begin == job.end()) {
    RAISE(kRuntimeError, "missing field: sources");
  }

  Vector<RefPtr<MapReduceTask>> sources;
  auto nsrc_begin = json::arrayLength(src_begin, job.end());
  for (size_t i = 0; i < nsrc_begin; ++i) {
    auto src_id = json::arrayGetString(src_begin, job.end(), i); // O(N^2) but who cares...
    if (src_id.isEmpty()) {
      RAISE(kRuntimeError, "illegal source definition");
    }

    sources.emplace_back(getJob(src_id.get(), shards, job_definitions, jobs));
  }

  return new SaveToTablePartitionTask(
      session_,
      table_name.get(),
      SHA1Hash::fromHexString(partition_key.get()),
      sources,
      shards,
      auth_,
      repl_);
}

} // namespace eventql

