/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "zbase/mapreduce/tasks/ReduceTask.h"
#include "zbase/mapreduce/MapReduceScheduler.h"
#include <algorithm>

using namespace stx;

namespace zbase {

ReduceTask::ReduceTask(
    const AnalyticsSession& session,
    RefPtr<MapReduceJobSpec> job_spec,
    const String& reduce_fn,
    const String& globals,
    const String& params,
    Vector<RefPtr<MapReduceTask>> sources,
    size_t num_shards,
    MapReduceShardList* shards,
    AnalyticsAuth* auth,
    zbase::ReplicationScheme* repl) :
    session_(session),
    job_spec_(job_spec),
    reduce_fn_(reduce_fn),
    globals_(globals),
    params_(params),
    sources_(sources),
    num_shards_(num_shards),
    auth_(auth),
    repl_(repl) {
  Vector<size_t> input;

  for (const auto& src : sources_) {
    auto src_indexes = src->shards();
    input.insert(input.end(), src_indexes.begin(), src_indexes.end());
  }

  for (size_t shard_idx = 0; shard_idx < num_shards_; shard_idx++) {
    auto shard = mkRef(new ReduceTaskShard());
    shard->task = this;
    shard->shard = shard_idx;
    shard->dependencies = input;
    addShard(shard.get(), shards);
  }
}

Option<MapReduceShardResult> ReduceTask::execute(
    RefPtr<MapReduceTaskShard> shard_base,
    RefPtr<MapReduceScheduler> job) {
  auto shard = shard_base.asInstanceOf<ReduceTaskShard>();

  Vector<String> input_tables;
  for (const auto& input : shard->dependencies) {
    auto input_tbl = job->getResultURL(input);
    if (input_tbl.isEmpty()) {
      continue;
    }

    input_tables.emplace_back(
        StringUtil::format(
            "$0?sample=$1:$2",
            input_tbl.get(),
            num_shards_,
            shard->shard));
  }

  std::sort(input_tables.begin(), input_tables.end());
  auto placement_id = SHA1::compute(
      StringUtil::format(
          "$0~$1",
          StringUtil::join(input_tables, "~"),
          shard->shard));

  Vector<String> errors;
  auto hosts = repl_->replicasFor(placement_id);
  for (const auto& host : hosts) {
    try {
      return executeRemote(shard.get(), job, input_tables, host);
    } catch (const StandardException& e) {
      logError(
          "z1.mapreduce",
          e,
          "ReduceTask::execute failed");

      errors.emplace_back(e.what());
    }
  }

  RAISEF(
      kRuntimeError,
      "ReduceTask::execute failed: $0",
      StringUtil::join(errors, ", "));
}

Option<MapReduceShardResult> ReduceTask::executeRemote(
    RefPtr<MapReduceTaskShard> shard,
    RefPtr<MapReduceScheduler> job,
    const Vector<String>& input_tables,
    const ReplicaRef& host) {
  logDebug(
      "z1.mapreduce",
      "Executing remote reduce shard on $2; customer=$0 input_tables=$1",
      session_.customer(),
      input_tables.size(),
      host.addr.hostAndPort());

  auto url = StringUtil::format(
      "http://$0/api/v1/mapreduce/tasks/reduce",
      host.addr.ipAndPort());

  auto params = StringUtil::format(
      "program_source=$0&reduce_fn=$1&globals=$2&params=$3",
      URI::urlEncode(job_spec_->program_source),
      URI::urlEncode(reduce_fn_),
      URI::urlEncode(globals_),
      URI::urlEncode(params_));

  for (const auto& input_table : input_tables) {
    params += "&input_table=" + URI::urlEncode(input_table);
  }

  auto api_token = auth_->encodeAuthToken(session_);

  http::HTTPMessage::HeaderList auth_headers;
  auth_headers.emplace_back(
      "Authorization",
      StringUtil::format("Token $0", api_token));

  http::HTTPClient http_client;
  auto req = http::HTTPRequest::mkPost(url, params, auth_headers);
  auto res = http_client.executeRequest(req);

  if (res.statusCode() == 204) {
    return None<MapReduceShardResult>();
  }

  if (res.statusCode() != 201) {
    RAISEF(
        kRuntimeError,
        "received non-201 response: $0", res.body().toString());
  }

  MapReduceShardResult result {
    .host = host,
    .result_id = SHA1Hash::fromHexString(res.body().toString())
  };

  return Some(result);
}

} // namespace zbase

