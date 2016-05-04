/**
 * This file is part of the "tsdb" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/http/httpconnectionpool.h>
#include <eventql/core/Partition.h>
#include <eventql/core/ReplicationScheme.h>
#include <eventql/core/ReplicationState.h>
#include <eventql/core/RecordEnvelope.pb.h>

using namespace stx;

namespace zbase {

class PartitionReplication : public RefCounted {
public:
  static const char kStateFileName[];

  PartitionReplication(
      RefPtr<Partition> partition,
      RefPtr<ReplicationScheme> repl_scheme,
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
  virtual bool replicate() = 0;

  /**
   * Returns the number of remote hosts (not including self!) for which we have
   * positively confirmed that they have a full copy of the data for this
   * partition
   *
   * IMPORTANT: it is imperative that this method only count hosts for that we
   * have positively confirmed that they have indeed received and acknowledged
   * a full copy of all the data. this invariant can never be broken since we
   * will _drop_ our local copy of the data based on the return value of this
   * method.
   */
  virtual size_t numFullRemoteCopies() const = 0;

  static ReplicationState fetchReplicationState(
      RefPtr<PartitionSnapshot> snap);

  ReplicationState fetchReplicationState() const;

protected:

  void commitReplicationState(const ReplicationState& state);

  RefPtr<Partition> partition_;
  RefPtr<PartitionSnapshot> snap_;
  RefPtr<ReplicationScheme> repl_scheme_;
  http::HTTPConnectionPool* http_;
};

} // namespace tdsb

