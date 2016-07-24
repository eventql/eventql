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
#include "eventql/util/http/HTTPSSEResponseHandler.h"
#include "eventql/mapreduce/tasks/ReduceTask.h"
#include "eventql/mapreduce/MapReduceScheduler.h"
#include <eventql/server/server_stats.h>
#include <eventql/db/server_allocator.h>
#include "eventql/eventql.h"

namespace eventql {

ReduceTask::ReduceTask(
    Session* session,
    const String& reduce_fn,
    const String& globals,
    const String& params,
    Vector<RefPtr<MapReduceTask>> sources,
    size_t num_shards,
    MapReduceShardList* shards,
    InternalAuth* auth,
    ConfigDirectory* cdir) :
    session_(session),
    reduce_fn_(reduce_fn),
    globals_(globals),
    params_(params),
    sources_(sources),
    num_shards_(num_shards),
    auth_(auth),
    cdir_(cdir) {
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

  Vector<String> errors;
  Set<String> servers;
  auto repl_factor = cdir_->getClusterConfig().replication_factor();
  ServerAllocator alloc(cdir_);
  auto rc = alloc.allocateServers(repl_factor, &servers);
  if (!rc.isSuccess()) {
    RAISEF(kRuntimeError, "error while allocating servers: $0", rc.message());
  }

  if (servers.empty()) {
    RAISE(kRuntimeError, "no available servers");
  }

  for (const auto& host : servers) {
    try {
      return executeRemote(shard.get(), job, input_tables, host);
    } catch (const StandardException& e) {
      logError(
          "evqld",
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
    const String& server_id) {

  auto server_cfg = cdir_->getServerConfig(server_id);
  if (server_cfg.server_status() != SERVER_UP) {
    RAISE(kRuntimeError, "server is down");
  }

  logDebug(
      "evqld",
      "Executing remote reduce shard on $2; customer=$0 input_tables=$1",
      session_->getEffectiveNamespace(),
      input_tables.size(),
      server_id);

  auto url = StringUtil::format(
      "http://$0/api/v1/mapreduce/tasks/reduce",
      server_cfg.server_addr());

  auto params = StringUtil::format(
      "reduce_fn=$0&globals=$1&params=$2",
      URI::urlEncode(reduce_fn_),
      URI::urlEncode(globals_),
      URI::urlEncode(params_));

  for (const auto& input_table : input_tables) {
    params += "&input_table=" + URI::urlEncode(input_table);
  }

  Option<MapReduceShardResult> result;
  Vector<String> errors;
  auto event_handler = [&] (const http::HTTPSSEEvent& ev) {
    if (ev.name.isEmpty()) {
      return;
    }

    if (ev.name.get() == "result_id") {
      result = Some(MapReduceShardResult {
        .server_id = server_id,
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

  http::HTTPClient http_client(&evqld_stats()->http_client_stats);
  auto req = http::HTTPRequest::mkPost(url, params);
  auth_->signRequest(session_, &req);

  auto res = http_client.executeRequest(
      req,
      http::HTTPSSEResponseHandler::getFactory(event_handler));

  if (!errors.empty()) {
    RAISE(kRuntimeError, StringUtil::join(errors, "; "));
  }

  if (res.statusCode() != 200) {
    RAISEF(kRuntimeError, "HTTP error: $0", url);
  }

  return result;
}

} // namespace eventql

