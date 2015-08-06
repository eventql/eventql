/**
 * This file is part of the "tsdb" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <tsdb/StaticPartitionReplication.h>
#include <tsdb/ReplicationScheme.h>
#include <stx/logging.h>
#include <stx/io/fileutil.h>
#include <stx/protobuf/msg.h>

using namespace stx;

namespace tsdb {

StaticPartitionReplication::StaticPartitionReplication(
    RefPtr<Partition> partition,
    RefPtr<ReplicationScheme> repl_scheme,
    http::HTTPConnectionPool* http) :
    PartitionReplication(partition, repl_scheme, http) {}

bool StaticPartitionReplication::needsReplication() const {
  return false;
}

bool StaticPartitionReplication::replicate() {
  return true;
}

} // namespace tdsb

