/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/SHA1.h>
#include <stx/autoref.h>
#include <stx/random.h>
#include <stx/option.h>
#include <stx/net/inetaddr.h>
#include <zbase/core/ClusterConfig.pb.h>

using namespace stx;

namespace zbase {

struct ReplicaRef {
  ReplicaRef(
      SHA1Hash _unique_id,
      InetAddr _addr);

  SHA1Hash unique_id;
  InetAddr addr;
  String name;
};

class ReplicationScheme : public RefCounted {
public:

  virtual Vector<ReplicaRef> replicasFor(const SHA1Hash& key) = 0;

  Vector<InetAddr> replicaAddrsFor(const SHA1Hash& key);

  virtual bool hasLocalReplica(const SHA1Hash& key) = 0;

};

/**
 * Single node has all the data, no replicas
 */
class StandaloneReplicationScheme : public ReplicationScheme {
public:
  Vector<ReplicaRef> replicasFor(const SHA1Hash& key) override;

  bool hasLocalReplica(const SHA1Hash& key) override;

};

/**
 * Data is replicated N-way. Every host has a full copy of all the data
 */
class FixedReplicationScheme : public ReplicationScheme {
public:

  FixedReplicationScheme(Vector<ReplicaRef> replicas);

  Vector<ReplicaRef> replicasFor(const SHA1Hash& key) override;

  bool hasLocalReplica(const SHA1Hash& key) override;

protected:
  Vector<ReplicaRef> replicas_;
};


/**
 * Data is replicated on N remote nodes (every remote host has a full copy
 * of the data) but the local node doesn't store any data
 */
class FrontendReplicationScheme : public ReplicationScheme {
public:

  FrontendReplicationScheme(Vector<ReplicaRef> replicas);

  Vector<ReplicaRef> replicasFor(const SHA1Hash& key) override;

  bool hasLocalReplica(const SHA1Hash& key) override;

protected:
  Vector<ReplicaRef> replicas_;
};


class DHTReplicationScheme : public ReplicationScheme {
public:

  DHTReplicationScheme(
      ClusterConfig cluster_config,
      Option<String> local_replica = None<String>());

  Vector<ReplicaRef> replicasFor(const SHA1Hash& key) override;

  bool hasLocalReplica(const SHA1Hash& key) override;

  void updateClusterConfig(ClusterConfig cluster_config);

protected:
  ClusterConfig cluster_config_;
  Option<String> local_replica_;
  OrderedMap<SHA1Hash, ReplicaRef> ring_;
};


} // namespace zbase

