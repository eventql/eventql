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
#include "eventql/eventql.h"
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/db/PartitionWriter.h>
#include <eventql/db/partition_arena.h>
#include <eventql/util/util/PersistentHashSet.h>
#include <eventql/db/CompactionStrategy.h>
#include <eventql/db/metadata_transaction.h>

namespace eventql {

class LSMPartitionWriter : public PartitionWriter {
public:
  static const size_t kDefaultPartitionSplitThresholdBytes = 1024llu * 1024llu * 512llu;
  static const size_t kMaxArenaRecords = 1024 * 64;
  static const size_t kMaxLSMTables = 12;

  LSMPartitionWriter(
      ServerCfg* cfg,
      RefPtr<Partition> partition,
      PartitionSnapshotRef* head);

  Set<SHA1Hash> insertRecords(
      const ShreddedRecordList& records) override;

  bool commit() override;
  bool needsCommit();
  bool needsUrgentCommit();

  bool compact() override;
  bool needsCompaction() override;
  bool needsUrgentCompaction();

  bool needsSplit() const;
  Status split();

  Status applyMetadataChange(
      const PartitionDiscoveryResponse& discovery_info) override;

  ReplicationState fetchReplicationState() const;
  void commitReplicationState(const ReplicationState& state);

protected:

  RefPtr<Partition> partition_;
  RefPtr<CompactionStrategy> compaction_strategy_;
  LSMTableIndexCache* idx_cache_;
  ConfigDirectory* cdir_;
  ReplicationScheme* repl_;
  size_t partition_split_threshold_;
  std::mutex commit_mutex_;
  std::mutex compaction_mutex_;
  std::mutex metadata_mutex_;
  std::mutex split_mutex_;
};

} // namespace tdsb

