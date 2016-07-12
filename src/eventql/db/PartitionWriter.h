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
#include <eventql/db/PartitionSnapshot.h>
#include <eventql/db/RecordRef.h>
#include <eventql/db/metadata_operations.pb.h>
#include <eventql/db/shredded_record.h>

namespace eventql {

class PartitionWriter : public RefCounted {
public:
  static const size_t kDefaultMaxDatafileSize = 1024 * 1024 * 128;

  PartitionWriter(PartitionSnapshotRef* head);

  virtual Set<SHA1Hash> insertRecords(
      const ShreddedRecordList& records) = 0;

  void updateCSTable(
      const String& tmpfile,
      uint64_t version);

  /**
   * Lock this partition writer (so that all write attempts will block/hang
   * until the writer is unlocked
   */
  void lock();

  /**
   * Unlock this partition writer
   */
  void unlock();

  /**
   * Freeze this partition writer, i.e. make it immutable. Every write attempt
   * on a frozen partition writer will return an error. Freezing a partition
   * can not be undone. (Freezing is used in the partition unload/delete process
   * to make sure writes to old references of a deleted partition that users
   * might have kepy around will fail)
   */
  void freeze();

  virtual bool needsCompaction() = 0;

  virtual bool commit() = 0;
  virtual bool compact() = 0;

  virtual Status applyMetadataChange(
      const PartitionDiscoveryResponse& discovery_info) = 0;

protected:
  PartitionSnapshotRef* head_;
  std::mutex mutex_;
  std::atomic<bool> frozen_;
};

} // namespace tdsb

