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
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/SHA1.h>
#include <eventql/util/autoref.h>
#include <eventql/util/random.h>
#include <eventql/util/option.h>
#include <eventql/util/net/inetaddr.h>
#include <eventql/db/ClusterConfig.pb.h>

#include "eventql/eventql.h"

namespace eventql {

struct ReplicaRef {
  ReplicaRef(
      SHA1Hash _unique_id,
      String _addr);

  SHA1Hash unique_id;
  String addr;
  String name;
  bool is_local;
};

class ReplicationScheme : public RefCounted {
public:

  virtual Vector<ReplicaRef> replicasFor(const SHA1Hash& key) = 0;

  Vector<String> replicaAddrsFor(const SHA1Hash& key);

  virtual bool hasLocalReplica(const SHA1Hash& key) = 0;

  virtual size_t minNumCopies() const = 0;

};

/**
 * Single node has all the data, no replicas
 */
class StandaloneReplicationScheme : public ReplicationScheme {
public:
  Vector<ReplicaRef> replicasFor(const SHA1Hash& key) override;

  bool hasLocalReplica(const SHA1Hash& key) override;

  size_t minNumCopies() const override;

};

/**
 * Data is replicated N-way. Every host has a full copy of all the data
 */
class FixedReplicationScheme : public ReplicationScheme {
public:

  /**
   * Construct with a list of all hosts in the cluster, including the local host.
   * Make sure is_local is set for the local host.
   */
  FixedReplicationScheme(Vector<ReplicaRef> replicas);

  Vector<ReplicaRef> replicasFor(const SHA1Hash& key) override;

  bool hasLocalReplica(const SHA1Hash& key) override;

  size_t minNumCopies() const override;

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

  size_t minNumCopies() const override;

  void updateClusterConfig(ClusterConfig cluster_config);

protected:
  ClusterConfig cluster_config_;
  Option<String> local_replica_;
  OrderedMap<SHA1Hash, ReplicaRef> ring_;
};


} // namespace eventql

