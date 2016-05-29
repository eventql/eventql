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
#include <eventql/db/PartitionWriter.h>
#include <eventql/db/RecordArena.h>
#include <eventql/util/util/PersistentHashSet.h>
#include <eventql/db/CompactionStrategy.h>
#include <eventql/db/metadata_transaction.h>
#include <eventql/db/metadata_operations.pb.h>

#include "eventql/eventql.h"

namespace eventql {

class LSMPartitionWriter : public PartitionWriter {
public:
  static const size_t kDefaultMaxDatafileSize = 1024 * 1024 * 128;
  static const size_t kMaxArenaRecords = 10000;

  LSMPartitionWriter(
      ServerCfg* cfg,
      RefPtr<Partition> partition,
      PartitionSnapshotRef* head);

  Set<SHA1Hash> insertRecords(
      const Vector<RecordRef>& records) override;

  bool commit() override;
  bool needsCommit();
  bool needsUrgentCommit();

  bool compact() override;
  bool needsCompaction() override;
  bool needsUrgentCompaction();

  Status applyMetadataChange(
      const PartitionDiscoveryResponse& discovery_info);

  ReplicationState fetchReplicationState() const;
  void commitReplicationState(const ReplicationState& state);

protected:

  void writeArenaToDisk(
      RefPtr<RecordArena> arena,
      uint64_t sequence,
      const String& filename);

  RefPtr<Partition> partition_;
  RefPtr<CompactionStrategy> compaction_strategy_;
  LSMTableIndexCache* idx_cache_;
  size_t max_datafile_size_;
  std::mutex commit_mutex_;
  std::mutex compaction_mutex_;
};

} // namespace tdsb

