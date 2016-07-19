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
#include <eventql/db/StaticPartitionWriter.h>
#include <eventql/util/logging.h>
#include <eventql/io/sstable/SSTableWriter.h>

#include "eventql/eventql.h"

namespace eventql {

StaticPartitionWriter::StaticPartitionWriter(
    PartitionSnapshotRef* head) :
    PartitionWriter(head) {}

Set<SHA1Hash> StaticPartitionWriter::insertRecords(
    const ShreddedRecordList& records) {
  RAISE(
      kRuntimeError,
      "can't insert individual records because partition is STATIC");
}

bool StaticPartitionWriter::needsCompaction() {
  return false;
}

bool StaticPartitionWriter::commit() {
 return true; // noop
}

bool StaticPartitionWriter::compact() {
 return true; // noop
}

} // namespace tdsb
