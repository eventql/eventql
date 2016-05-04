/**
 * This file is part of the "tsdb" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <eventql/core/PartitionReplication.h>
#include <eventql/core/ReplicationScheme.h>
#include <stx/logging.h>
#include <stx/io/fileutil.h>
#include <stx/protobuf/msg.h>

using namespace stx;

namespace zbase {

const char PartitionReplication::kStateFileName[] = "_repl";

PartitionReplication::PartitionReplication(
    RefPtr<Partition> partition,
    RefPtr<ReplicationScheme> repl_scheme,
    http::HTTPConnectionPool* http) :
    partition_(partition),
    snap_(partition_->getSnapshot()),
    repl_scheme_(repl_scheme),
    http_(http) {}

ReplicationState PartitionReplication::fetchReplicationState(
    RefPtr<PartitionSnapshot> snap) {
  auto filepath = FileUtil::joinPaths(snap->base_path, kStateFileName);
  String tbl_uuid((char*) snap->uuid().data(), snap->uuid().size());

  if (FileUtil::exists(filepath)) {
    auto disk_state = msg::decode<ReplicationState>(FileUtil::read(filepath));

    if (disk_state.uuid() == tbl_uuid) {
      return disk_state;
    }
  }

  ReplicationState state;
  state.set_uuid(tbl_uuid);
  return state;
}

ReplicationState PartitionReplication::fetchReplicationState() const {
  return fetchReplicationState(snap_);
}

void PartitionReplication::commitReplicationState(
    const ReplicationState& state) {
  auto filepath = FileUtil::joinPaths(snap_->base_path, kStateFileName);
  auto buf = msg::encode(state);

  {
    auto file = File::openFile(
        filepath + "~",
        File::O_WRITE | File::O_CREATEOROPEN | File::O_TRUNCATE);

    file.write(buf->data(), buf->size());
  }

  FileUtil::mv(filepath + "~", filepath);
}

} // namespace tdsb

