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
#include <eventql/util/http/httpconnectionpool.h>
#include <eventql/db/Partition.h>
#include <eventql/db/replication_state.h>
#include <eventql/db/RecordEnvelope.pb.h>

#include "eventql/eventql.h"

namespace eventql {

class ReplicationInfo;
class PartitionReplication : public RefCounted {
public:
  static const char kStateFileName[];

  PartitionReplication(
      RefPtr<Partition> partition,
      http::HTTPConnectionPool* http);

  /**
   * Returns true if we need still need to replicate some data to one or more
   * remote host and false if we positively copied all data to all remote
   * replicas which need to have a copy.
   *
   * IMPORTANT: it is imperative that this method never returns false unless
   * we positively confirmed that every other replica has indeed received and
   * acknowledged a full copy of all the data. this invariant can never be broken
   * since we will _drop_ our local copy of the data based on the return value
   * of this method.
   */
  virtual bool needsReplication() const = 0;

  /**
   * Tries to replicate this partition to all other remote nodes to which it
   * still needs to be replicated. Returns true on success, false on error
   */
  virtual bool replicate(ReplicationInfo* replication_info) = 0;

  static ReplicationState fetchReplicationState(
      RefPtr<PartitionSnapshot> snap);

  ReplicationState fetchReplicationState() const;

  /**
   * IMPORTANT: it is imperative that this method only count hosts for that we
   * have positively confirmed that they have indeed received and acknowledged
   * a full copy of all the data. this invariant can never be broken since we
   * will _drop_ our local copy of the data based on the return value of this
   * method.
   */
  virtual bool shouldDropPartition() const = 0;

protected:

  void commitReplicationState(const ReplicationState& state);

  RefPtr<Partition> partition_;
  RefPtr<PartitionSnapshot> snap_;
  http::HTTPConnectionPool* http_;
};

} // namespace tdsb

