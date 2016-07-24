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
#include "eventql/util/SHA1.h"
#include "eventql/util/http/HTTPSSEResponseHandler.h"
#include "eventql/mapreduce/tasks/MapTableTask.h"
#include "eventql/mapreduce/MapReduceScheduler.h"
#include "eventql/db/metadata_client.h"
#include <eventql/server/server_stats.h>
#include <eventql/server/sql/table_provider.h>

#include "eventql/eventql.h"

namespace eventql {

MapTableTask::MapTableTask(
    Session* session,
    const TSDBTableRef& table_ref,
    const String& map_function,
    const String& globals,
    const String& params,
    MapReduceShardList* shards,
    InternalAuth* auth,
    eventql::PartitionMap* pmap,
    eventql::ConfigDirectory* cdir,
    eventql::ReplicationScheme* repl) :
    session_(session),
    table_ref_(table_ref),
    map_function_(map_function),
    globals_(globals),
    params_(params),
    auth_(auth),
    pmap_(pmap),
    cdir_(cdir),
    repl_(repl) {
  auto table = pmap_->findTable(session_->getEffectiveNamespace(), table_ref_.table_key);
  if (table.isEmpty()) {
    RAISEF(kNotFoundError, "table not found: $0", table_ref_.table_key);
  }

  auto keyspace = table.get()->getKeyspaceType();
  KeyRange kr;

  if (!table_ref_.keyrange_begin.isEmpty()) {
    kr.begin = encodePartitionKey(keyspace, table_ref_.keyrange_begin.get());
  }

  if (!table_ref_.keyrange_limit.isEmpty()) {
    kr.end = encodePartitionKey(keyspace, table_ref_.keyrange_limit.get());
  }

  MetadataClient metadata_client(cdir_);
  PartitionListResponse partition_list;
  auto rc = metadata_client.listPartitions(
      session->getEffectiveNamespace(),
      table_ref_.table_key,
      kr,
      &partition_list);

  if (!rc.isSuccess()) {
    RAISEF(kRuntimeError, "metadata lookup failure: $0", rc.message());
  }

  for (const auto& p : partition_list.partitions()) {
    Vector<ReplicaRef> replicas;
    SHA1Hash pid(p.partition_id().data(), p.partition_id().size());

    for (const auto& s : p.servers()) {
      auto server_cfg = cdir_->getServerConfig(s);
      if (server_cfg.server_status() != SERVER_UP) {
        continue;
      }

      ReplicaRef rref(SHA1::compute(s), server_cfg.server_addr());
      rref.name = s;
      replicas.emplace_back(rref);
    }

    auto shard = mkRef(new MapTableTaskShard());
    shard->task = this;
    shard->table_ref = table_ref_;
    shard->table_ref.partition_key = pid;
    shard->servers = replicas;
    addShard(shard.get(), shards);
  }
}

Option<MapReduceShardResult> MapTableTask::execute(
    RefPtr<MapReduceTaskShard> shard_base,
    RefPtr<MapReduceScheduler> job) {
  auto shard = shard_base.asInstanceOf<MapTableTaskShard>();

  Vector<String> errors;
  for (const auto& host : shard->servers) {
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
      session_->getEffectiveNamespace(),
      shard->table_ref.table_key,
      shard->table_ref.partition_key.get().toString(),
      host.addr);

  auto url = StringUtil::format(
      "http://$0/api/v1/mapreduce/tasks/map_partition",
      host.addr);

  auto params = StringUtil::format(
      "table=$0&partition=$1&map_function=$2&globals=$3&params=$4&required_columns=$5",
      URI::urlEncode(shard->table_ref.table_key),
      shard->table_ref.partition_key.get().toString(),
      URI::urlEncode(map_function_),
      URI::urlEncode(globals_),
      URI::urlEncode(params_),
      URI::urlEncode(StringUtil::join(required_columns_, ",")));

  Option<MapReduceShardResult> result;
  Vector<String> errors;
  auto event_handler = [&] (const http::HTTPSSEEvent& ev) {
    if (ev.name.isEmpty()) {
      return;
    }

    if (ev.name.get() == "result_id") {
      result = Some(MapReduceShardResult {
        .host = host,
        .result_id = SHA1Hash::fromHexString(ev.data)
      });
    }

    else if (ev.name.get() == "log") {
      job->sendLogline(URI::urlDecode(ev.data));
    }

    else if (ev.name.get() == "error") {
      errors.emplace_back(ev.data);
    }
  };

  http::HTTPClient http_client(&z1stats()->http_client_stats);
  auto req = http::HTTPRequest::mkPost(url, params);
  auth_->signRequest(session_, &req);

  auto res = http_client.executeRequest(
      req,
      http::HTTPSSEResponseHandler::getFactory(event_handler));

  if (!errors.empty()) {
    RAISE(kRuntimeError, StringUtil::join(errors, "; "));
  }

  if (res.statusCode() != 200) {
    RAISEF(kRuntimeError, "HTTP Error: $0", url);
  }

  return result;
}

void MapTableTask::setRequiredColumns(const Set<String>& columns) {
  required_columns_ = columns;
}

} // namespace eventql

