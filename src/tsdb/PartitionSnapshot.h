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
#include <stx/autoref.h>
#include <stx/SHA1.h>
#include <tsdb/PartitionState.pb.h>

using namespace stx;

namespace tsdb {
class Table;

struct PartitionSnapshot : public RefCounted {

  PartitionSnapshot(
      const PartitionState& state,
      const String& _base_path,
      size_t _nrecs);

  RefPtr<PartitionSnapshot> clone() const;
  void writeToDisk();

  SHA1Hash uuid() const;

  SHA1Hash key;
  PartitionState state;
  const String base_path;
  uint64_t nrecs;
};

class PartitionSnapshotRef {
public:

  PartitionSnapshotRef(RefPtr<PartitionSnapshot> snap);

  RefPtr<PartitionSnapshot> getSnapshot() const;
  void setSnapshot(RefPtr<PartitionSnapshot> snap);

protected:
  RefPtr<PartitionSnapshot> snap_;
  mutable std::mutex mutex_;
};

}
