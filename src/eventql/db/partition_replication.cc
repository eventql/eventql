/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#include <eventql/db/partition_replication.h>
#include <eventql/util/logging.h>
#include <eventql/util/io/fileutil.h>
#include <eventql/util/protobuf/msg.h>

#include "eventql/eventql.h"

namespace eventql {

const char PartitionReplication::kStateFileName[] = "_repl";

PartitionReplication::PartitionReplication(
    RefPtr<Partition> partition,
    http::HTTPConnectionPool* http) :
    partition_(partition),
    snap_(partition_->getSnapshot()),
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

