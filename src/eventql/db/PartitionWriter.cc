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
#include <eventql/util/io/fileutil.h>
#include <eventql/db/Partition.h>
#include <eventql/db/PartitionWriter.h>
#include <eventql/util/logging.h>
#include <eventql/io/sstable/SSTableWriter.h>

#include "eventql/eventql.h"

namespace eventql {

PartitionWriter::PartitionWriter(
    PartitionSnapshotRef* head) :
    head_(head),
    frozen_(false) {}

//bool PartitionWriter::insertRecord(
//    const SHA1Hash& record_id,
//    uint64_t record_version,
//    const Buffer& record) {
//  Vector<RecordRef> recs;
//  recs.emplace_back(record_id, record_version, record);
//  auto ids = insertRecords(recs);
//  return !ids.empty();
//}

void PartitionWriter::updateCSTable(
    const String& tmpfile,
    uint64_t version) {
  std::unique_lock<std::mutex> lk(mutex_);
  if (frozen_) {
    RAISE(kIllegalStateError, "partition is frozen");
  }

  auto snap = head_->getSnapshot()->clone();

  if (snap->state.cstable_version() >= version) {
    return;
    //RAISE(
    //    kRuntimeError,
    //    "refusing cstable update because the update version is less than or " \
    //    "equal to the head version");
  }

  logDebug(
      "tsdb",
      "Updating cstable for partition $0/$1/$2",
      snap->state.tsdb_namespace(),
      snap->state.table_key(),
      snap->key.toString());

  auto filepath = FileUtil::joinPaths(snap->base_path, "_cstable");
  FileUtil::mv(tmpfile, filepath);

  snap->state.set_cstable_version(version);
  snap->writeToDisk();
  head_->setSnapshot(snap);
}

void PartitionWriter::lock() {
  mutex_.lock();
}

void PartitionWriter::unlock() {
  mutex_.unlock();
}

void PartitionWriter::freeze() {
  frozen_ = true;
}

} // namespace tdsb
