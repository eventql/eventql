/**
 * This file is part of the "tsdb" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/autoref.h>
#include <zbase/core/PartitionSnapshot.h>
#include <zbase/core/RecordRef.h>

using namespace stx;

namespace zbase {

class PartitionWriter : public RefCounted {
public:
  static const size_t kDefaultMaxDatafileSize = 1024 * 1024 * 128;

  PartitionWriter(PartitionSnapshotRef* head);

  bool insertRecord(
      const SHA1Hash& record_id,
      uint64_t record_version,
      const Buffer& record);

  virtual Set<SHA1Hash> insertRecords(
      const Vector<RecordRef>& records) = 0;

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

protected:
  PartitionSnapshotRef* head_;
  std::mutex mutex_;
  std::atomic<bool> frozen_;
};

} // namespace tdsb

