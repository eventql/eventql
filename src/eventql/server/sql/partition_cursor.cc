/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *   - Laura Schlimmer <laura@zscale.io>
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
#include "eventql/server/sql/partition_cursor.h"

namespace eventql {

PartitionCursor::PartitionCursor(
    csql::Transaction* txn,
    csql::ExecutionContext* execution_context,
    RefPtr<Table> table,
    RefPtr<PartitionSnapshot> snap,
    RefPtr<csql::SequentialScanNode> stmt) :
    txn_(txn),
    execution_context_(execution_context),
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
      cur_scan_.reset(
          new csql::CSTableScan(txn_, execution_context_, stmt_, cstable));

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
