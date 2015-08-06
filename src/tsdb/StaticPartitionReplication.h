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
#include <tsdb/PartitionReplication.h>

using namespace stx;

namespace tsdb {

class StaticPartitionReplication : public PartitionReplication {
public:

  StaticPartitionReplication(
      RefPtr<Partition> partition,
      RefPtr<ReplicationScheme> repl_scheme,
      http::HTTPConnectionPool* http);

  bool needsReplication() const override;

  /**
   * Returns true on success, false on error
   */
  bool replicate() override;

};

} // namespace tdsb

