/**
 * This file is part of the "tsdb" project
 *   Copyright (c) 2015 Paul Asmuth, FnordCorp B.V.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/fnv.h>
#include <stx/io/fileutil.h>
#include <stx/protobuf/MessageDecoder.h>
#include <sstable/sstablereader.h>
#include <zbase/core/LSMPartitionReader.h>
#include <zbase/core/LSMPartitionSQLScan.h>
#include <zbase/core/Table.h>

using namespace stx;

namespace zbase {

LSMPartitionReader::LSMPartitionReader(
    RefPtr<Table> table,
    RefPtr<PartitionSnapshot> head) :
    PartitionReader(head),
    table_(table) {}

void LSMPartitionReader::fetchRecords(
    Function<void (const msg::MessageObject& record)> fn) {
  RAISE(kNotYetImplementedError, "not yet implemented");
}

SHA1Hash LSMPartitionReader::version() const {
  return SHA1::compute(StringUtil::toString(snap_->nrecs));
}

ScopedPtr<csql::TableExpression> LSMPartitionReader::buildSQLScan(
    RefPtr<csql::SequentialScanNode> node,
    csql::QueryBuilder* runtime) const {
  auto scan = mkScoped(
      new LSMPartitionSQLScan(
          table_,
          snap_,
          node,
          runtime));

  //auto cstable_version = cstableVersion();
  //if (!cstable_version.isEmpty()) {
  //  scan->setCacheKey(cstable_version.get());
  //}

  return std::move(scan);
}

} // namespace tdsb

