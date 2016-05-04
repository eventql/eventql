/**
 * This file is part of the "tsdb" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/io/file.h>
#include <stx/io/fileutil.h>
#include <stx/protobuf/msg.h>
#include <eventql/core/PartitionSnapshot.h>
#include <eventql/core/Table.h>

using namespace stx;

namespace zbase {

PartitionSnapshot::PartitionSnapshot(
    const PartitionState& _state,
    const String& _abs_path,
    const String& _rel_path,
    size_t _nrecs) :
    key(
        _state.partition_key().data(),
        _state.partition_key().size()),
    state(_state),
    base_path(_abs_path),
    rel_path(_rel_path),
    nrecs(_nrecs),
    head_arena(new RecordArena()) {}

SHA1Hash PartitionSnapshot::uuid() const {
  return SHA1Hash(state.uuid().data(), state.uuid().size());
}

RefPtr<PartitionSnapshot> PartitionSnapshot::clone() const {
  auto snap = mkRef(new PartitionSnapshot(
      state,
      base_path,
      rel_path,
      nrecs));

  snap->head_arena = head_arena;
  snap->compacting_arena = compacting_arena;
  return snap;
}

void PartitionSnapshot::writeToDisk() {
  auto fpath = FileUtil::joinPaths(base_path, "_snapshot");

  {
    auto f = File::openFile(
        fpath + "~",
        File::O_WRITE | File::O_CREATEOROPEN | File::O_TRUNCATE);

    auto buf = msg::encode(state);
    f.write(buf->data(), buf->size());
  }

  FileUtil::mv(fpath + "~", fpath);
}

PartitionSnapshotRef::PartitionSnapshotRef(
    RefPtr<PartitionSnapshot> snap) :
    snap_(snap) {}

RefPtr<PartitionSnapshot> PartitionSnapshotRef::getSnapshot() const {
  std::unique_lock<std::mutex> lk(mutex_);
  auto snap = snap_;
  return snap;
}

void PartitionSnapshotRef::setSnapshot(RefPtr<PartitionSnapshot> snap) {
  std::unique_lock<std::mutex> lk(mutex_);
  snap_ = snap;
}

}
