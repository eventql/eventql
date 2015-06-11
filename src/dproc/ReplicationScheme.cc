/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <dproc/ReplicationScheme.h>
#include <fnord/fnv.h>

using namespace fnord;

namespace dproc {

Vector<ReplicaRef> StandaloneReplicationScheme::replicasFor(const String& key) {
  return Vector<ReplicaRef>{};
}

bool StandaloneReplicationScheme::keepLocalReplicaFor(const String& key) {
  return true;
}


void FixedReplicationScheme::addHost(uint64_t unique_id, const String& addr) {
  replicas_.emplace_back(ReplicaRef {
    .unique_id = unique_id,
    .addr = addr
  });
}

void FixedReplicationScheme::addHost(const String& addr) {
  FNV<uint64_t> fnv;
  addHost(fnv.hash(addr.data(), addr.size()), addr);
}

Vector<ReplicaRef> FixedReplicationScheme::replicasFor(const String& key) {
  return replicas_;
}

bool FixedReplicationScheme::keepLocalReplicaFor(const String& key) {
  return true;
}

} // namespace dproc
