/**
 * This file is part of the "tsdb" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <tsdb/ReplicationState.h>

using namespace stx;

namespace tsdb {

uint64_t replicatedOffsetFor(
    const ReplicationState& repl_state,
    const SHA1Hash& replica_id) {
  for (const auto& replica : repl_state.replicas()) {
    SHA1Hash rid(replica.replica_id().data(), replica.replica_id().size());
    if (rid == replica_id) {
      return replica.replicated_offset();
    }
  }

  return 0;
}

void setReplicatedOffsetFor(
    ReplicationState* repl_state,
    const SHA1Hash& replica_id,
    uint64_t replicated_offset) {
  for (auto& replica : *repl_state->mutable_replicas()) {
    SHA1Hash rid(replica.replica_id().data(), replica.replica_id().size());
    if (rid == replica_id) {
      replica.set_replicated_offset(replicated_offset);
      return;
    }
  }

  auto replica = repl_state->add_replicas();
  replica->set_replica_id(replica_id.data(), replica_id.size());
  replica->set_replicated_offset(replicated_offset);
}

uint64_t replicatedVersionFor(
    const ReplicationState& repl_state,
    const SHA1Hash& replica_id) {
  for (const auto& replica : repl_state.replicas()) {
    SHA1Hash rid(replica.replica_id().data(), replica.replica_id().size());
    if (rid == replica_id) {
      return replica.version();
    }
  }

  return 0;
}

void setReplicatedVersionFor(
    ReplicationState* repl_state,
    const SHA1Hash& replica_id,
    uint64_t version) {
  for (auto& replica : *repl_state->mutable_replicas()) {
    SHA1Hash rid(replica.replica_id().data(), replica.replica_id().size());
    if (rid == replica_id) {
      replica.set_version(version);
      return;
    }
  }

  auto replica = repl_state->add_replicas();
  replica->set_replica_id(replica_id.data(), replica_id.size());
  replica->set_version(version);
}

} // namespace tdsb

