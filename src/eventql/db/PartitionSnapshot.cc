/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
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
#include <eventql/util/io/file.h>
#include <eventql/util/io/fileutil.h>
#include <eventql/util/protobuf/msg.h>
#include <eventql/db/PartitionSnapshot.h>
#include <eventql/db/Table.h>

#include "eventql/eventql.h"

namespace eventql {

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
