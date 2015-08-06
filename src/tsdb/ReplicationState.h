/**
 * This file is part of the "tsdb" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/SHA1.h>
#include <tsdb/ReplicationState.pb.h>

using namespace stx;

namespace tsdb {

uint64_t replicatedOffsetFor(
    const ReplicationState& repl_state,
    const SHA1Hash& replica_id);

void setReplicatedOffsetFor(
    ReplicationState* repl_state,
    const SHA1Hash& replica_id,
    uint64_t replicated_offset);

uint64_t replicatedVersionFor(
    const ReplicationState& repl_state,
    const SHA1Hash& replica_id);

void setReplicatedVersionFor(
    ReplicationState* repl_state,
    const SHA1Hash& replica_id,
    uint64_t version);

} // namespace tdsb

