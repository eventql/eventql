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
#include "eventql/mapreduce/tasks/SaveToTablePartitionTask.h"
#include "eventql/mapreduce/MapReduceScheduler.h"
#include "eventql/db/FixedShardPartitioner.h"
#include "eventql/io/sstable/sstablereader.h"
#include <eventql/server/server_stats.h>

#include "eventql/eventql.h"

namespace eventql {

SaveToTablePartitionTask::SaveToTablePartitionTask(
    const AnalyticsSession& session,
    const String& table_name,
    const SHA1Hash& partition_key,
    Vector<RefPtr<MapReduceTask>> sources,
    MapReduceShardList* shards,
    AnalyticsAuth* auth,
    eventql::ReplicationScheme* repl) :
    session_(session),
    table_name_(table_name),
    partition_key_(partition_key),
    sources_(sources),
    auth_(auth),
    repl_(repl) {
  Vector<size_t> input;
  for (const auto& src : sources_) {
    auto src_indexes = src->shards();
    input.insert(input.end(), src_indexes.begin(), src_indexes.end());
  }

  auto shard = mkRef(new MapReduceTaskShard());
  shard->task = this;
  shard->dependencies = input;
  addShard(shard.get(), shards);
}

Option<MapReduceShardResult> SaveToTablePartitionTask::execute(
    RefPtr<MapReduceTaskShard> shard,
    RefPtr<MapReduceScheduler> job) {
  Vector<String> input_tables;
  for (const auto& input : shard->dependencies) {
    auto input_tbl = job->getResultURL(input);
    if (input_tbl.isEmpty()) {
      continue;
    }

    input_tables.emplace_back(input_tbl.get());
  }

  std::sort(input_tables.begin(), input_tables.end());

  Vector<String> errors;
  auto hosts = repl_->replicasFor(partition_key_);
  for (const auto& host : hosts) {
    try {
      return executeRemote(shard.get(), job, input_tables, host);
    } catch (const StandardException& e) {
      logError(
          "z1.mapreduce",
          e,
          "SaveToTablePartitionTask::execute failed");

      errors.emplace_back(e.what());
    }
  }

  RAISEF(
      kRuntimeError,
      "SaveToTablePartitionTask::execute failed: $0",
      StringUtil::join(errors, ", "));
}

Option<MapReduceShardResult> SaveToTablePartitionTask::executeRemote(
    RefPtr<MapReduceTaskShard> shard,
    RefPtr<MapReduceScheduler> job,
    const Vector<String>& input_tables,
    const ReplicaRef& host) {
  logDebug(
      "z1.mapreduce",
      "Saving result to table partition; target=$0/$1/$2 host=$3",
      session_.customer(),
      table_name_,
      partition_key_.toString(),
      host.addr.hostAndPort());

  auto url = StringUtil::format(
      "http://$0/api/v1/mapreduce/tasks/save_to_table_partition",
      host.addr.ipAndPort());

  auto params = StringUtil::format(
      "table_name=$0&partition=$1",
      URI::urlEncode(table_name_),
      partition_key_.toString());

  for (const auto& input_table : input_tables) {
    params += "&input_table=" + URI::urlEncode(input_table);
  }

  auto api_token = auth_->encodeAuthToken(session_);

  http::HTTPMessage::HeaderList auth_headers;
  auth_headers.emplace_back(
      "Authorization",
      StringUtil::format("Token $0", api_token));

  http::HTTPClient http_client(&z1stats()->http_client_stats);
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

  //MapReduceShardResult result {
  //  .host = host,
  //  .result_id = SHA1Hash::fromHexString(res.body().toString())
  //};

  //return Some(result);

  return None<MapReduceShardResult>();
}

} // namespace eventql

