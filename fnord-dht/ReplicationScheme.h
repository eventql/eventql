/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_DHT_REPLICATIONSCHEME_H
#define _FNORD_DHT_REPLICATIONSCHEME_H
#include <fnord-base/stdtypes.h>
#include <fnord-base/autoref.h>
#include <fnord-base/random.h>
#include <fnord-base/option.h>

namespace fnord {
namespace dht {

struct ReplicaRef {
  uint64_t unique_id;
  String addr;
};

class ReplicationScheme : public RefCounted {
public:
  virtual Vector<ReplicaRef> replicasFor(const String& key) = 0;
  virtual bool keepLocalReplicaFor(const String& key) = 0;
};

/**
 * Single node has all the data, no replicas
 */
class StandaloneReplicationScheme : public ReplicationScheme {
public:
  Vector<ReplicaRef> replicasFor(const String& key) override;
  bool keepLocalReplicaFor(const String& key) override;
};

/**
 * Data is replicated N-way. Every host has a full copy of all the data
 */
class FixedReplicationScheme : public ReplicationScheme {
public:

  void addHost(uint64_t unique_id, const String& addr);

  Vector<ReplicaRef> replicasFor(const String& key) override;
  bool keepLocalReplicaFor(const String& key) override;

protected:
  Vector<ReplicaRef> replicas_;
};


} // namespace dht
} // namespace fnord

#endif
