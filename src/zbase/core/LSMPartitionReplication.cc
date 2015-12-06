/**
 * This file is part of the "tsdb" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <zbase/core/LSMPartitionReplication.h>
#include <zbase/core/LSMPartitionReader.h>
#include <zbase/core/ReplicationScheme.h>
#include <stx/logging.h>
#include <stx/io/fileutil.h>
#include <stx/protobuf/msg.h>

using namespace stx;

namespace zbase {

const size_t LSMPartitionReplication::kMaxBatchSizeRows = 8192;
const size_t LSMPartitionReplication::kMaxBatchSizeBytes = 1024 * 1024 * 50; // 50 MB

LSMPartitionReplication::LSMPartitionReplication(
    RefPtr<Partition> partition,
    RefPtr<ReplicationScheme> repl_scheme,
    http::HTTPConnectionPool* http) :
    PartitionReplication(partition, repl_scheme, http) {}

bool LSMPartitionReplication::needsReplication() const {
  auto replicas = repl_scheme_->replicasFor(snap_->key);
  if (replicas.size() == 0) {
    return false;
  }

  auto repl_state = fetchReplicationState();
  auto head_offset = snap_->nrecs;
  for (const auto& r : replicas) {
    if (r.is_local) {
      continue;
    }

    const auto& replica_offset = replicatedOffsetFor(repl_state, r.unique_id);
    if (replica_offset < head_offset) {
      return true;
    }
  }

  return false;
}

size_t LSMPartitionReplication::numFullRemoteCopies() const {
  size_t ncopies = 0;
  auto replicas = repl_scheme_->replicasFor(snap_->key);
  auto repl_state = fetchReplicationState();
  auto head_offset = snap_->nrecs;

  for (const auto& r : replicas) {
    if (r.is_local) {
      continue;
    }

    const auto& replica_offset = replicatedOffsetFor(repl_state, r.unique_id);
    if (replica_offset >= head_offset) {
      ncopies += 1;
    }
  }

  return ncopies;
}

void LSMPartitionReplication::replicateTo(
    const ReplicaRef& replica,
    uint64_t replicated_offset) {
  if (replica.is_local) {
    RAISE(kIllegalStateError, "can't replicate to myself");
  }
}

bool LSMPartitionReplication::replicate() {
  auto replicas = repl_scheme_->replicasFor(snap_->key);
  if (replicas.size() == 0) {
    return true;
  }

  auto repl_state = fetchReplicationState();
  auto head_offset = snap_->nrecs;
  bool dirty = false;
  bool success = true;

  for (const auto& r : replicas) {
    if (r.is_local) {
      continue;
    }

    const auto& replica_offset = replicatedOffsetFor(repl_state, r.unique_id);

    if (replica_offset < head_offset) {
      logDebug(
          "z1.replication",
          "Replicating partition $0/$1/$2 to $3 ($4 records)",
          snap_->state.tsdb_namespace(),
          snap_->state.table_key(),
          snap_->key.toString(),
          r.addr.hostAndPort(),
          head_offset - replica_offset);

      try {
        replicateTo(r, replica_offset);
        setReplicatedOffsetFor(&repl_state, r.unique_id, head_offset);
        dirty = true;
      } catch (const std::exception& e) {
        success = false;

        stx::logError(
          "z1.replication",
          e,
          "Error while replicating partition $0/$1/$2 to $3",
          snap_->state.tsdb_namespace(),
          snap_->state.table_key(),
          snap_->key.toString(),
          r.addr.hostAndPort());
      }
    }
  }

  if (dirty) {
    commitReplicationState(repl_state);
  }

  return success;
}

} // namespace tdsb

