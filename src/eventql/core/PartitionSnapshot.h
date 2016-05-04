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
#include <eventql/core/PartitionState.pb.h>
#include <eventql/core/RecordArena.h>

using namespace stx;

namespace zbase {
class Table;

struct PartitionSnapshot : public RefCounted {

  PartitionSnapshot(
      const PartitionState& state,
      const String& _abs_path,
      const String& _rel_path,
      size_t _nrecs);

  RefPtr<PartitionSnapshot> clone() const;
  void writeToDisk();

  SHA1Hash uuid() const;

  SHA1Hash key;
  PartitionState state;
  const String base_path;
  const String rel_path;
  uint64_t nrecs;
  RefPtr<RecordArena> head_arena;
  RefPtr<RecordArena> compacting_arena;
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
