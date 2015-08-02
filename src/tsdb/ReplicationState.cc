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

} // namespace tdsb

