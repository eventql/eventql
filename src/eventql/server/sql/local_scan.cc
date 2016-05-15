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
#include <eventql/sql/table_scan.h>

using namespace util;

namespace eventql {

TableScan::TableScan(
    csql::Transaction* txn,
    RefPtr<Table> table,
    RefPtr<PartitionSnapshot> snap,
    RefPtr<csql::SequentialScanNode> stmt,
    csql::QueryBuilder* runtime,
    csql::RowSinkFn output) :
    txn_(txn),
    table_(table),
    snap_(snap),
    stmt_(stmt),
    runtime_(runtime),
    output_(output) {}

//void TableScan::onInputsReady() {
//  switch (table_->storage()) {
//    case TBL_STORAGE_COLSM:
//      scanLSMTable();
//      return;
//    case TBL_STORAGE_STATIC:
//      scanStaticTable();
//      return;
//  }
//}

void TableScan::scanLSMTable() {
  const auto& tables = snap_->state.lsm_tables();
  for (auto tbl = tables.rbegin(); tbl != tables.rend(); ++tbl) {
    auto cstable_file = FileUtil::joinPaths(
        snap_->base_path,
        tbl->filename() + ".cst");
    auto cstable = cstable::CSTableReader::openFile(cstable_file);
    auto id_col = cstable->getColumnReader("__lsm_id");
    auto is_update_col = cstable->getColumnReader("__lsm_is_update");
    csql::CSTableScan cstable_scan(txn_, stmt_, cstable, runtime_, output_);
    cstable_scan.open();

    cstable_scan.setFilter([this, id_col, is_update_col] () -> bool {
      uint64_t rlvl;
      uint64_t dlvl;
      String id_str;
      id_col->readString(&rlvl, &dlvl, &id_str);
      bool is_update;
      is_update_col->readBoolean(&rlvl, &dlvl, &is_update);
      SHA1Hash id(id_str.data(), id_str.size());
      if (id_set_.count(id) > 0) {
        return false;
      } else {
        if (is_update) {
          id_set_.emplace(id);
        }
        return true;
      }
    });

    for (const auto& col : table_->schema()->columns()) {
      switch (col.second.type) {
        case msg::FieldType::BOOLEAN:
          cstable_scan.setColumnType(col.first, SQL_BOOL);
          break;
        case msg::FieldType::UINT32:
          cstable_scan.setColumnType(col.first, SQL_INTEGER);
          break;
        case msg::FieldType::UINT64:
          cstable_scan.setColumnType(col.first, SQL_INTEGER);
          break;
        case msg::FieldType::STRING:
          cstable_scan.setColumnType(col.first, SQL_STRING);
          break;
        case msg::FieldType::DOUBLE:
          cstable_scan.setColumnType(col.first, SQL_FLOAT);
          break;
        case msg::FieldType::DATETIME:
          cstable_scan.setColumnType(col.first, SQL_TIMESTAMP);
          break;
      }
    }

    //cstable_scan.onInputsReady();
  }

  id_set_.clear();
}

void TableScan::scanStaticTable() {
  auto cstable_file = FileUtil::joinPaths(snap_->base_path, "_cstable");

  if (FileUtil::exists(cstable_file)) {
    auto cstable = cstable::CSTableReader::openFile(cstable_file);
    csql::CSTableScan cstable_scan(txn_, stmt_, cstable, runtime_, output_);
    cstable_scan.open();

    for (const auto& col : table_->schema()->columns()) {
      switch (col.second.type) {
        case msg::FieldType::BOOLEAN:
          cstable_scan.setColumnType(col.first, SQL_BOOL);
          break;
        case msg::FieldType::UINT32:
          cstable_scan.setColumnType(col.first, SQL_INTEGER);
          break;
        case msg::FieldType::UINT64:
          cstable_scan.setColumnType(col.first, SQL_INTEGER);
          break;
        case msg::FieldType::STRING:
          cstable_scan.setColumnType(col.first, SQL_STRING);
          break;
        case msg::FieldType::DOUBLE:
          cstable_scan.setColumnType(col.first, SQL_FLOAT);
          break;
        case msg::FieldType::DATETIME:
          cstable_scan.setColumnType(col.first, SQL_TIMESTAMP);
          break;
      }
    }

    //cstable_scan.onInputsReady();
  }
}

int TableScan::nextRow(csql::SValue* out, int out_len) {
  return -1;
}

int EmptyTableScan::nextRow(csql::SValue* out, int out_len) {
  return -1;
}

TableScanFactory::TableScanFactory(
    PartitionMap* pmap,
    String keyspace,
    String table,
    SHA1Hash partition,
    RefPtr<csql::SequentialScanNode> stmt) :
    pmap_(pmap),
    keyspace_(keyspace),
    table_(table),
    partition_(partition),
    stmt_(stmt) {}

RefPtr<csql::Task> TableScanFactory::build(
    csql::Transaction* txn,
    csql::RowSinkFn output) const {
  auto table = pmap_->findTable(keyspace_, table_);
  if (table.isEmpty()) {
    RAISEF(kRuntimeError, "table not found: '$0'", table_);
  }

  auto partition = pmap_->findPartition(keyspace_, table_, partition_);
  if (partition.isEmpty()) {
    return new EmptyTableScan();
  }

  return new TableScan(
      txn,
      table.get(),
      partition.get()->getSnapshot(),
      stmt_,
      txn->getRuntime()->queryBuilder().get(),
      output);
}

} // namespace eventql
