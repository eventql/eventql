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
    const String& method_name,
    AnalyticsAuth* auth,
    zbase::PartitionMap* pmap,
    zbase::ReplicationScheme* repl) :
    session_(session),
    job_spec_(job_spec),
    table_ref_(table_ref),
    method_name_(method_name),
    auth_(auth),
    pmap_(pmap),
    repl_(repl) {}

Vector<size_t> MapTableTask::build(MapReduceShardList* shards) {
  auto table = pmap_->findTable(session_.customer(), table_ref_.table_key);
  if (table.isEmpty()) {
    RAISEF(kNotFoundError, "table not found: $0", table_ref_.table_key);
  }

  auto partitioner = table.get()->partitioner();
  auto partitions = partitioner->partitionKeysFor(table_ref_);

  Vector<size_t> indexes;
  for (const auto& partition : partitions) {
    auto shard = mkRef(new MapTableTaskShard());
    shard->task = this;
    shard->table_ref = table_ref_;
    shard->table_ref.partition_key = partition;

    indexes.emplace_back(shards->size());
    shards->emplace_back(shard.get());
  }

  return indexes;
}

MapReduceShardResult MapTableTask::execute(
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

MapReduceShardResult MapTableTask::executeRemote(
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
      "http://$0/api/v1/mapreduce/tasks/map_partition?table=$1&partition=$2" \
      "&program_source=$3&method_name=$4",
      host.addr.ipAndPort(),
      URI::urlEncode(shard->table_ref.table_key),
      shard->table_ref.partition_key.get().toString(),
      URI::urlEncode(job_spec_->program_source),
      URI::urlEncode(method_name_));

  auto api_token = auth_->encodeAuthToken(session_);

  http::HTTPMessage::HeaderList auth_headers;
  auth_headers.emplace_back(
      "Authorization",
      StringUtil::format("Token $0", api_token));

  http::HTTPClient http_client;
  auto req = http::HTTPRequest::mkGet(url, auth_headers);
  auto res = http_client.executeRequest(req);

  if (res.statusCode() != 201) {
    RAISEF(
        kRuntimeError,
        "received non-201 response: $0", res.body().toString());
  }

  MapReduceShardResult result {
    .host = host,
    .result_id = SHA1Hash::fromHexString(res.body().toString())
  };

  return result;
}

} // namespace zbase

