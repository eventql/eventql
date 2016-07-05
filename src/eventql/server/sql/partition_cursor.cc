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
  for (;;) {
    if (cur_cursor_.get() == nullptr) {
      if (!openNextTable()) {
        break;
      }
    }

    if (cur_cursor_->next(row, row_len)) {
      return true;
    } else {
      cur_cursor_.reset(nullptr);
    }
  }

  return false;
}

bool PartitionCursor::openNextTable() {
  RefPtr<cstable::CSTableReader> cstable;
  switch (cur_table_) {
    case 0: {
      if (snap_->head_arena.get() &&
          snap_->head_arena->getCSTableFile()) {
        cstable = cstable::CSTableReader::openFile(
            snap_->head_arena->getCSTableFile());
        break;
      } else {
        ++cur_table_;
        /* fallthrough */
      }
    }

    case 1: {
      if (snap_->compacting_arena.get() &&
          snap_->compacting_arena->getCSTableFile()) {
        cstable = cstable::CSTableReader::openFile(
            snap_->compacting_arena->getCSTableFile());
        break;
      } else {
        ++cur_table_;
        /* fallthrough */
      }
    }

    default: {
      if (cur_table_ >= snap_->state.lsm_tables().size() + 2) {
        return false;
      }

      const auto& tbl = (snap_->state.lsm_tables().data())[cur_table_ - 2];
      auto cstable_file = FileUtil::joinPaths(
          snap_->base_path,
          tbl->filename() + ".cst");
      cstable = cstable::CSTableReader::openFile(cstable_file);
      break;
    }
  }

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
      case msg::FieldType::OBJECT:
        break;
    }
  }

  ++cur_table_;
  cur_cursor_ = cur_scan_->execute();
  return true;
}

size_t PartitionCursor::getNumColumns() {
  return stmt_->getNumComputedColumns();
}

StaticPartitionCursor::StaticPartitionCursor(
    csql::Transaction* txn,
    csql::ExecutionContext* execution_context,
    RefPtr<Table> table,
    RefPtr<PartitionSnapshot> snap,
    RefPtr<csql::SequentialScanNode> stmt) :
    txn_(txn),
    execution_context_(execution_context),
    table_(table),
    snap_(snap),
    stmt_(stmt) {};

bool StaticPartitionCursor::next(csql::SValue* row, int row_len) {
  if (cur_cursor_.get() == nullptr) {
    auto cstable_file = FileUtil::joinPaths(snap_->base_path, "_cstable");
    if (!FileUtil::exists(cstable_file)) {
      return false;
    }

    auto cstable = cstable::CSTableReader::openFile(cstable_file);

    cur_scan_.reset(
        new csql::CSTableScan(txn_, execution_context_, stmt_, cstable));

    //for (const auto& col : table_->schema()->columns()) {
    //  switch (col.second.type) {
    //    case msg::FieldType::BOOLEAN:
    //      cur_scan_->setColumnType(col.first, SQL_BOOL);
    //      break;
    //    case msg::FieldType::UINT32:
    //      cur_scan_->setColumnType(col.first, SQL_INTEGER);
    //      break;
    //    case msg::FieldType::UINT64:
    //      cur_scan_->setColumnType(col.first, SQL_INTEGER);
    //      break;
    //    case msg::FieldType::STRING:
    //      cur_scan_->setColumnType(col.first, SQL_STRING);
    //      break;
    //    case msg::FieldType::DOUBLE:
    //      cur_scan_->setColumnType(col.first, SQL_FLOAT);
    //      break;
    //    case msg::FieldType::DATETIME:
    //      cur_scan_->setColumnType(col.first, SQL_TIMESTAMP);
    //      break;
    //  }
    //}

    cur_cursor_ = cur_scan_->execute();
  }

  return cur_cursor_->next(row, row_len);
}


size_t StaticPartitionCursor::getNumColumns() {
  return stmt_->getNumComputedColumns();
}

}
