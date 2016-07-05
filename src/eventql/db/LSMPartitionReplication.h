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
#include <eventql/db/PartitionReplication.h>
#include <eventql/db/PartitionState.pb.h>
#include <eventql/config/config_directory.h>

namespace eventql {

class LSMPartitionReplication : public PartitionReplication {
public:
  static const size_t kMaxBatchSizeBytes;
  static const size_t kMaxBatchSizeRows;

  LSMPartitionReplication(
      RefPtr<Partition> partition,
      RefPtr<ReplicationScheme> repl_scheme,
      ConfigDirectory* cdir,
      http::HTTPConnectionPool* http);

  bool needsReplication() const override;

  /**
   * Returns true on success, false on error
   */
  bool replicate(ReplicationInfo* replication_info) override;

  bool shouldDropPartition() const override;

protected:

  Status fetchAndApplyMetadataTransaction();
  Status fetchAndApplyMetadataTransaction(MetadataTransaction txn);
  Status finalizeSplit();
  Status finalizeJoin(const ReplicationTarget& target);

  void replicateTo(
      const ReplicationTarget& replica,
      uint64_t replicated_offset);

  void uploadBatchTo(
      const String& host,
      const RecordEnvelopeList& batch);

  void fetchRecords(
      size_t start_sequence,
      const String& keyrange_begin,
      const String& keyrange_end,
      Function<void (
          const SHA1Hash& record_id,
          uint64_t record_version,
          const void* record_data,
          size_t record_size)> fn);

  ConfigDirectory* cdir_;

};

} // namespace eventql

