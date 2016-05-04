/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <eventql/core/ReplicationScheme.h>
#include <stx/fnv.h>

using namespace stx;

namespace zbase {

ReplicaRef::ReplicaRef(
    SHA1Hash _unique_id,
    InetAddr _addr) :
    unique_id(_unique_id),
    addr(_addr),
    is_local(false) {}

Vector<InetAddr> ReplicationScheme::replicaAddrsFor(const SHA1Hash& key) {
  Vector<InetAddr> addrs;

  for (const auto& r : replicasFor(key)) {
    addrs.emplace_back(r.addr);
  }

  return addrs;
}


Vector<ReplicaRef> StandaloneReplicationScheme::replicasFor(
    const SHA1Hash& key) {
  return Vector<ReplicaRef>{};
}

bool StandaloneReplicationScheme::hasLocalReplica(const SHA1Hash& key) {
  return true;
}

size_t StandaloneReplicationScheme::minNumCopies() const {
  return 1;
}

FixedReplicationScheme::FixedReplicationScheme(
    Vector<ReplicaRef> replicas) :
    replicas_(replicas) {}

Vector<ReplicaRef> FixedReplicationScheme::replicasFor(const SHA1Hash& key) {
  return replicas_;
}

bool FixedReplicationScheme::hasLocalReplica(const SHA1Hash& key) {
  return true;
}

size_t FixedReplicationScheme::minNumCopies() const {
  return replicas_.size();
}

DHTReplicationScheme::DHTReplicationScheme(
    ClusterConfig cluster_config,
    Option<String> local_replica /* = None<String>() */) :
    cluster_config_(cluster_config),
    local_replica_(local_replica) {
  for (const auto& node : cluster_config_.dht_nodes()) {
    auto addr = InetAddr::resolve(node.addr());

    for (const auto& token_str : node.sha1_tokens()) {
      auto token = SHA1Hash::fromHexString(token_str);
      ReplicaRef rref(token, addr);
      rref.name = node.name();

      if (!local_replica.isEmpty() && node.name() == local_replica_.get()) {
        rref.is_local = true;
      }

      ring_.emplace(token, rref);
    }
  }
}

Vector<ReplicaRef> DHTReplicationScheme::replicasFor(const SHA1Hash& key) {
  Set<String> hosts;
  Vector<ReplicaRef> replicas;
  if (ring_.empty()) {
    return replicas;
  }

  auto ncopies = cluster_config_.dht_num_copies();
  auto begin = ring_.lower_bound(key);
  if (begin == ring_.end()) {
    begin = ring_.begin();
  }

  auto cur = begin;
  do {
    auto host = cur->second.addr.ipAndPort();

    if (hosts.count(host) == 0) {
      replicas.emplace_back(cur->second);
      hosts.emplace(host);
    }

    if (++cur == ring_.end()) {
      cur = ring_.begin();
    }
  } while (replicas.size() < ncopies && cur != begin);

  return replicas;
}

bool DHTReplicationScheme::hasLocalReplica(const SHA1Hash& key) {
  if (local_replica_.isEmpty()) {
    return false;
  }

  auto replicas = replicasFor(key);
  for (const auto& r : replicas) {
    if (r.is_local) {
      return true;
    }
  }

  return false;
}

size_t DHTReplicationScheme::minNumCopies() const {
  return cluster_config_.dht_num_copies();
}

} // namespace zbase
