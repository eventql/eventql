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
#include <tsdb/PartitionState.pb.h>

using namespace stx;

namespace tsdb {
class Table;

struct PartitionSnapshot : public RefCounted {

  PartitionSnapshot(
      const PartitionState& state,
      const String& _base_path,
      RefPtr<Table> _table,
      size_t _nrecs);

  RefPtr<PartitionSnapshot> clone() const;

  SHA1Hash key;
  PartitionState state;
  String base_path;
  RefPtr<Table> table;
  uint64_t nrecs;
};

}
