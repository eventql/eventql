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
#include <stx/stdtypes.h>
#include <stx/http/httpconnectionpool.h>
#include <zbase/core/Partition.h>
#include <zbase/core/ReplicationScheme.h>
#include <zbase/core/ReplicationState.h>
#include <zbase/core/RecordEnvelope.pb.h>

using namespace stx;

namespace zbase {

class PartitionReplication : public RefCounted {
public:
  static const char kStateFileName[];

  PartitionReplication(
      RefPtr<Partition> partition,
      RefPtr<ReplicationScheme> repl_scheme,
      http::HTTPConnectionPool* http);

  virtual bool needsReplication() const = 0;

  /**
   * Returns true on success, false on error
   */
  virtual bool replicate() = 0;

protected:

  ReplicationState fetchReplicationState() const;
  void commitReplicationState(const ReplicationState& state);

  RefPtr<Partition> partition_;
  RefPtr<PartitionSnapshot> snap_;
  RefPtr<ReplicationScheme> repl_scheme_;
  http::HTTPConnectionPool* http_;
};

} // namespace tdsb

