/**
 * Copyright (c) 2015 - zScale Technology GmbH <legal@zscale.io>
 *   All Rights Reserved.
 *
 * Authors:
 *   Paul Asmuth <paul@zscale.io>
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <zbase/core/LSMPartitionSQLScan.h>

using namespace stx;

namespace zbase {

LSMPartitionSQLScan::LSMPartitionSQLScan(
    RefPtr<Table> table,
    RefPtr<PartitionSnapshot> snap,
    RefPtr<csql::SequentialScanNode> stmt,
    csql::QueryBuilder* runtime) :
    table_(table),
    snap_(snap),
    stmt_(stmt),
    runtime_(runtime) {
  for (const auto& slnode : stmt->selectList()) {
    column_names_.emplace_back(slnode->columnName());
  }
}

Vector<String> LSMPartitionSQLScan::columnNames() const {
  return column_names_;
}

size_t LSMPartitionSQLScan::numColumns() const {
  return column_names_.size();
}

void LSMPartitionSQLScan::prepare(csql::ExecutionContext* context) {
  size_t ntasks = 0;
  ntasks += snap_->state.lsm_tables().size();
  context->incrNumSubtasksTotal(ntasks);
}

void LSMPartitionSQLScan::execute(
    csql::ExecutionContext* context,
    Function<bool (int argc, const csql::SValue* argv)> fn) {
  const auto& tables = snap_->state.lsm_tables();
  for (auto tbl = tables.rbegin(); tbl != tables.rend(); ++tbl) {
    auto cstable_file = FileUtil::joinPaths(
        snap_->base_path,
        tbl->filename() + ".cst");
    csql::CSTableScan cstable_scan(stmt_, cstable_file, runtime_);
    cstable_scan.execute(context, fn);
  }
}

Option<SHA1Hash> LSMPartitionSQLScan::cacheKey() const {
  return cache_key_;
}

void LSMPartitionSQLScan::setCacheKey(const SHA1Hash& key) {
  cache_key_ = Some(key);
}

} // namespace zbase
