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
    csql::Transaction* ctx,
    RefPtr<Table> table,
    RefPtr<PartitionSnapshot> snap,
    RefPtr<csql::SequentialScanNode> stmt,
    csql::QueryBuilder* runtime) :
    ctx_(ctx),
    table_(table),
    snap_(snap),
    stmt_(stmt),
    runtime_(runtime) {}

//void LSMPartitionSQLScan::execute(
//    csql::ExecutionContext* context,
//    Function<bool (int argc, const csql::SValue* argv)> fn) {
//  const auto& tables = snap_->state.lsm_tables();
//  for (auto tbl = tables.rbegin(); tbl != tables.rend(); ++tbl) {
//    auto cstable_file = FileUtil::joinPaths(
//        snap_->base_path,
//        tbl->filename() + ".cst");
//    auto cstable = cstable::CSTableReader::openFile(cstable_file);
//    auto id_col = cstable->getColumnReader("__lsm_id");
//    auto is_update_col = cstable->getColumnReader("__lsm_is_update");
//    csql::CSTableScan cstable_scan(ctx_, stmt_, cstable, runtime_);
//    cstable_scan.open();
//
//    cstable_scan.setFilter([this, id_col, is_update_col] () -> bool {
//      uint64_t rlvl;
//      uint64_t dlvl;
//      String id_str;
//      id_col->readString(&rlvl, &dlvl, &id_str);
//      bool is_update;
//      is_update_col->readBoolean(&rlvl, &dlvl, &is_update);
//      SHA1Hash id(id_str.data(), id_str.size());
//      if (id_set_.count(id) > 0) {
//        return false;
//      } else {
//        if (is_update) {
//          id_set_.emplace(id);
//        }
//        return true;
//      }
//    });
//
//    for (const auto& col : table_->schema()->columns()) {
//      switch (col.second.type) {
//        case msg::FieldType::BOOLEAN:
//          cstable_scan.setColumnType(col.first, SQL_BOOL);
//          break;
//        case msg::FieldType::UINT32:
//          cstable_scan.setColumnType(col.first, SQL_INTEGER);
//          break;
//        case msg::FieldType::UINT64:
//          cstable_scan.setColumnType(col.first, SQL_INTEGER);
//          break;
//        case msg::FieldType::STRING:
//          cstable_scan.setColumnType(col.first, SQL_STRING);
//          break;
//        case msg::FieldType::DOUBLE:
//          cstable_scan.setColumnType(col.first, SQL_FLOAT);
//          break;
//        case msg::FieldType::DATETIME:
//          cstable_scan.setColumnType(col.first, SQL_TIMESTAMP);
//          break;
//      }
//    }
//
//    cstable_scan.execute(context, fn);
//  }
//
//  id_set_.clear();
//}

} // namespace zbase
