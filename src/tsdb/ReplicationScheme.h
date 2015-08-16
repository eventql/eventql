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

using namespace stx;

namespace tsdb {

struct ReplicaRef {
  SHA1Hash unique_id;
  String addr;
};

class ReplicationScheme : public RefCounted {
public:

  virtual Vector<ReplicaRef> replicasFor(const SHA1Hash& key) = 0;

  virtual bool hasLocalReplica(const String& key) = 0;

};

/**
 * Single node has all the data, no replicas
 */
class StandaloneReplicationScheme : public ReplicationScheme {
public:
  Vector<ReplicaRef> replicasFor(const SHA1Hash& key) override;

  bool hasLocalReplica(const String& key) override;

};

/**
 * Data is replicated N-way. Every host has a full copy of all the data
 */
class FixedReplicationScheme : public ReplicationScheme {
public:

  FixedReplicationScheme(Vector<ReplicaRef> replicas);

  Vector<ReplicaRef> replicasFor(const SHA1Hash& key) override;

  bool hasLocalReplica(const String& key) override;

protected:
  Vector<ReplicaRef> replicas_;
};


} // namespace tsdb

