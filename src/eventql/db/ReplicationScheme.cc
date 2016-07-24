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
#include <eventql/db/ReplicationScheme.h>
#include <eventql/util/fnv.h>

#include "eventql/eventql.h"

namespace eventql {

ReplicaRef::ReplicaRef(
    SHA1Hash _unique_id,
    String _addr) :
    unique_id(_unique_id),
    addr(_addr),
    is_local(false) {}

Vector<String> ReplicationScheme::replicaAddrsFor(const SHA1Hash& key) {
  Vector<String> addrs;

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
    auto addr = node.addr();
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

  auto ncopies = cluster_config_.replication_factor();
  auto begin = ring_.lower_bound(key);
  if (begin == ring_.end()) {
    begin = ring_.begin();
  }

  auto cur = begin;
  do {
    auto host = cur->second.addr;

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
  return cluster_config_.replication_factor();
}

} // namespace eventql
