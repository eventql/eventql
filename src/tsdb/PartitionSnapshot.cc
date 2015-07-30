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
#include <tsdb/PartitionSnapshot.h>
#include <tsdb/Table.h>

using namespace stx;

namespace tsdb {

PartitionSnapshot::PartitionSnapshot(
    const PartitionState& _state,
    const String& _base_path,
    RefPtr<Table> _table,
    size_t _nrecs) :
    key(
        state.partition_key().data(),
        state.partition_key().size()),
    state(_state),
    base_path(_base_path),
    table(_table),
    nrecs(_nrecs) {}

RefPtr<PartitionSnapshot> PartitionSnapshot::clone() const {
  return new PartitionSnapshot(
      state,
      base_path,
      table,
      nrecs);
}

void PartitionSnapshot::writeToDisk() {
  auto fpath = StringUtil::format(
      "$0/$1.snx",
      base_path,
      key.toString());
  {
    auto f = File::openFile(
        fpath + "~",
        File::O_WRITE | File::O_CREATEOROPEN | File::O_TRUNCATE);

    auto buf = msg::encode(state);
    f.write(buf->data(), buf->size());
  }

  FileUtil::mv(fpath + "~", fpath);
}

}
