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
#include <eventql/db/StaticPartitionReplication.h>
#include <eventql/db/ReplicationScheme.h>
#include <eventql/db/TSDBClient.h>
#include <eventql/util/logging.h>
#include <eventql/util/io/fileutil.h>
#include <eventql/util/protobuf/msg.h>

#include "eventql/eventql.h"

namespace eventql {

StaticPartitionReplication::StaticPartitionReplication(
    RefPtr<Partition> partition,
    RefPtr<ReplicationScheme> repl_scheme,
    http::HTTPConnectionPool* http) :
    PartitionReplication(partition, repl_scheme, http) {}

bool StaticPartitionReplication::needsReplication() const {
  auto replicas = repl_scheme_->replicasFor(snap_->key);
  if (replicas.size() == 0) {
    return false;
  }

  auto repl_state = fetchReplicationState();
  auto head_version = snap_->state.cstable_version();
  for (const auto& r : replicas) {
    if (r.is_local) {
      continue;
    }

    const auto& replica_version = replicatedVersionFor(repl_state, r.unique_id);
    if (replica_version < head_version) {
      return true;
    }
  }

  return false;
}

size_t StaticPartitionReplication::numFullRemoteCopies() const {
  size_t ncopies = 0;
  auto replicas = repl_scheme_->replicasFor(snap_->key);
  auto repl_state = fetchReplicationState();
  auto head_version = snap_->state.cstable_version();

  for (const auto& r : replicas) {
    if (r.is_local) {
      continue;
    }

    const auto& replica_version = replicatedVersionFor(repl_state, r.unique_id);
    if (replica_version >= head_version) {
      ncopies += 1;
    }
  }

  return ncopies;
}

void StaticPartitionReplication::replicateTo(
    const ReplicaRef& replica,
    uint64_t head_version) {
  if (replica.is_local) {
    RAISE(kIllegalStateError, "can't replicate to myself");
  }

  auto tsdb_url = StringUtil::format("http://$0/tsdb", replica.addr);
  eventql::TSDBClient tsdb_client(tsdb_url, http_);

  auto pinfo = tsdb_client.partitionInfo(
      snap_->state.tsdb_namespace(),
      snap_->state.table_key(),
      snap_->key);

  if (!pinfo.isEmpty() && pinfo.get().cstable_version() >= head_version) {
    return;
  }

  auto reader = partition_->getReader();
  auto cstable_file = FileUtil::joinPaths(snap_->base_path, "_cstable");
  if (!FileUtil::exists(cstable_file)) {
    return;
  }

  auto uri = URI(
      StringUtil::format(
          "$0/update_cstable?namespace=$1&table=$2&partition=$3&version=$4",
          tsdb_url,
          URI::urlEncode(snap_->state.tsdb_namespace()),
          URI::urlEncode(snap_->state.table_key()),
          snap_->key.toString(),
          head_version));

  http::HTTPRequest req(http::HTTPMessage::M_POST, uri.pathAndQuery());
  req.addHeader("Host", uri.hostAndPort());
  req.addBody(FileUtil::read(cstable_file)); // FIXME use FileUpload

  auto res = http_->executeRequest(req);
  res.wait();

  const auto& r = res.get();
  if (r.statusCode() != 201) {
    RAISEF(kRuntimeError, "received non-201 response: $0", r.body().toString());
  }
}

bool StaticPartitionReplication::replicate() {
  auto replicas = repl_scheme_->replicasFor(snap_->key);
  if (replicas.size() == 0) {
    return true;
  }

  auto repl_state = fetchReplicationState();
  auto head_version = snap_->state.cstable_version();
  bool dirty = false;
  bool success = true;

  for (const auto& r : replicas) {
    if (r.is_local) {
      continue;
    }

    const auto& replica_version = replicatedVersionFor(repl_state, r.unique_id);

    if (replica_version < head_version) {
      logTrace(
          "tsdb",
          "Replicating partition $0/$1/$2 to $3",
          snap_->state.tsdb_namespace(),
          snap_->state.table_key(),
          snap_->key.toString(),
          r.addr);

      try {
        replicateTo(r, head_version);
        setReplicatedVersionFor(&repl_state, r.unique_id, head_version);
        dirty = true;

        logTrace(
            "tsdb",
            "Replicating done: $0/$1/$2 to $3",
            snap_->state.tsdb_namespace(),
            snap_->state.table_key(),
            snap_->key.toString(),
            r.addr);
      } catch (const std::exception& e) {
        success = false;

        logError(
            "tsdb",
            e,
            "Error while replicating partition $0/$1/$2 to $3",
            snap_->state.tsdb_namespace(),
            snap_->state.table_key(),
            snap_->key.toString(),
            r.addr);
      }
    }
  }

  if (dirty) {
    commitReplicationState(repl_state);
  }

  return success;
}

} // namespace tdsb

