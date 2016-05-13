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
#include "eventql/server/sql/partition_cursor.h"

namespace zbase {

PartitionCursor::PartitionCursor(
    csql::Transaction* txn,
    RefPtr<Table> table,
    RefPtr<PartitionSnapshot> snap,
    RefPtr<csql::SequentialScanNode> stmt) :
    txn_(txn),
    table_(table),
    snap_(snap),
    stmt_(stmt),
    cur_table_(0) {};

bool PartitionCursor::next(csql::SValue* row, int row_len) {
  while (cur_table_ < snap_->state.lsm_tables().size()) {
    if (cur_cursor_.get() == nullptr) {
      const auto& tbl = (snap_->state.lsm_tables().data())[cur_table_];
      auto cstable_file = FileUtil::joinPaths(
          snap_->base_path,
          tbl->filename() + ".cst");
      auto cstable = cstable::CSTableReader::openFile(cstable_file);
      auto id_col = cstable->getColumnReader("__lsm_id");
      auto is_update_col = cstable->getColumnReader("__lsm_is_update");
      cur_scan_.reset(new csql::CSTableScan(txn_, stmt_, cstable));

      cur_scan_->setFilter([this, id_col, is_update_col] () -> bool {
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
            cur_scan_->setColumnType(col.first, SQL_BOOL);
            break;
          case msg::FieldType::UINT32:
            cur_scan_->setColumnType(col.first, SQL_INTEGER);
            break;
          case msg::FieldType::UINT64:
            cur_scan_->setColumnType(col.first, SQL_INTEGER);
            break;
          case msg::FieldType::STRING:
            cur_scan_->setColumnType(col.first, SQL_STRING);
            break;
          case msg::FieldType::DOUBLE:
            cur_scan_->setColumnType(col.first, SQL_FLOAT);
            break;
          case msg::FieldType::DATETIME:
            cur_scan_->setColumnType(col.first, SQL_TIMESTAMP);
            break;
        }
      }

      cur_cursor_ = cur_scan_->execute();
    }

    if (cur_cursor_->next(row, row_len)) {
      return true;
    } else {
      cur_cursor_.reset(nullptr);
      ++cur_table_;
    }
  }

  return false;
}

size_t PartitionCursor::getNumColumns() {
  return stmt_->numColumns();
}

}
