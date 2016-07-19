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
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/util/SHA1.h>
#include <eventql/db/PartitionState.pb.h>
#include <eventql/db/partition_arena.h>
#include <eventql/db/ServerConfig.h>
#include "eventql/eventql.h"

namespace eventql {
class Table;

struct PartitionSnapshot : public RefCounted {

  PartitionSnapshot(
      const Table* table,
      const PartitionState& state,
      const String& _abs_path,
      const String& _rel_path,
      ServerCfg* _server_cfg,
      size_t _nrecs);

  PartitionSnapshot(
      const PartitionState& state,
      const String& _abs_path,
      const String& _rel_path,
      ServerCfg* _server_cfg,
      size_t _nrecs);

  ~PartitionSnapshot();

  RefPtr<PartitionSnapshot> clone() const;
  void writeToDisk();

  SHA1Hash uuid() const;

  SHA1Hash key;
  PartitionState state;
  const String base_path;
  const String rel_path;
  ServerCfg* server_cfg;
  uint64_t nrecs;
  RefPtr<PartitionArena> head_arena;
  RefPtr<PartitionArena> compacting_arena;
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
