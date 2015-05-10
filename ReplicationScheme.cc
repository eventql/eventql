/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord-dht/ReplicationScheme.h>

namespace fnord {
namespace dht {

Vector<ReplicaRef> StandaloneReplicationScheme::replicasFor(const String& key) {
  return Vector<ReplicaRef>{};
}

bool StandaloneReplicationScheme::keepLocalReplicaFor(const String& key) {
  return true;
}

} // namespace dht
} // namespace fnord

