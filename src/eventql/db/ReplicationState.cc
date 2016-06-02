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
#include "eventql/eventql.h"
#include <eventql/db/ReplicationState.h>

namespace eventql {

uint64_t replicatedOffsetFor(
    const ReplicationState& repl_state,
    const ReplicationTarget& target) {
  return replicatedOffsetFor(
      repl_state,
      SHA1::compute(
          StringUtil::format(
              "$0~$1",
              target.server_id(),
              target.placement_id())));
}

void setReplicatedOffsetFor(
    ReplicationState* repl_state,
    const ReplicationTarget& target,
    uint64_t replicated_offset) {
  setReplicatedOffsetFor(
      repl_state,
      SHA1::compute(
          StringUtil::format(
              "$0~$1",
              target.server_id(),
              target.placement_id())),
      replicated_offset);
}

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

