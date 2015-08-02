/**
 * This file is part of the "tsdb" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <tsdb/PartitionReplication.h>

using namespace stx;

namespace tsdb {

bool PartitionReplication::needsReplication(RefPtr<Partition> partition) {
  return false;
}

void PartitionReplication::replicate(RefPtr<Partition> partition) {
  auto snap = partition->getSnapshot();
  auto table = partition->getTable();

  logDebug(
      "tsdb",
      "Replicating partition $0/$1/$2",
      snap->state.tsdb_namespace(),
      table->name(),
      snap->key.toString());
}


} // namespace tdsb

