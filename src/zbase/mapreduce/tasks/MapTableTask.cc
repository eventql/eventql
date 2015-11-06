/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "stx/SHA1.h"
#include "zbase/mapreduce/tasks/MapTableTask.h"
#include "zbase/mapreduce/MapReduceScheduler.h"

using namespace stx;

namespace zbase {

MapTableTask::MapTableTask(
    const AnalyticsSession& session,
    RefPtr<MapReduceJobSpec> job_spec,
    const TSDBTableRef& table_ref,
    const String& map_function,
    MapReduceShardList* shards,
    AnalyticsAuth* auth,
    zbase::PartitionMap* pmap,
    zbase::ReplicationScheme* repl) :
    session_(session),
    job_spec_(job_spec),
    table_ref_(table_ref),
    map_function_(map_function),
    auth_(auth),
    pmap_(pmap),
    repl_(repl) {
  auto table = pmap_->findTable(session_.customer(), table_ref_.table_key);
  if (table.isEmpty()) {
    RAISEF(kNotFoundError, "table not found: $0", table_ref_.table_key);
  }

  auto partitioner = table.get()->partitioner();
  auto partitions = partitioner->partitionKeysFor(table_ref_);

  for (const auto& partition : partitions) {
    auto shard = mkRef(new MapTableTaskShard());
    shard->task = this;
    shard->table_ref = table_ref_;
    shard->table_ref.partition_key = partition;

    addShard(shard.get(), shards);
  }
}

Option<MapReduceShardResult> MapTableTask::execute(
    RefPtr<MapReduceTaskShard> shard_base,
    RefPtr<MapReduceScheduler> job) {
  auto shard = shard_base.asInstanceOf<MapTableTaskShard>();

  Vector<String> errors;
  auto hosts = repl_->replicasFor(shard->table_ref.partition_key.get());
  for (const auto& host : hosts) {
    try {
      return executeRemote(shard, job, host);
    } catch (const StandardException& e) {
      logError(
          "z1.mapreduce",
          e,
          "MapTableTask::execute failed");

      errors.emplace_back(e.what());
    }
  }

  RAISEF(
      kRuntimeError,
      "MapTableTask::execute failed: $0",
      StringUtil::join(errors, ", "));
}

Option<MapReduceShardResult> MapTableTask::executeRemote(
    RefPtr<MapTableTaskShard> shard,
    RefPtr<MapReduceScheduler> job,
    const ReplicaRef& host) {
  logDebug(
      "z1.mapreduce",
      "Executing map table shard on $0/$1/$2 on $3",
      session_.customer(),
      shard->table_ref.table_key,
      shard->table_ref.partition_key.get().toString(),
      host.addr.hostAndPort());

  auto url = StringUtil::format(
      "http://$0/api/v1/mapreduce/tasks/map_partition",
      host.addr.ipAndPort());

  auto params = StringUtil::format(
      "table=$0&partition=$1&map_function=$2",
      URI::urlEncode(shard->table_ref.table_key),
      shard->table_ref.partition_key.get().toString(),
      URI::urlEncode(map_function_));


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

