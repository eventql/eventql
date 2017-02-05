/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *   - Laura Schlimmer <laura@eventql.io>
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
#include "eventql/sql/svalue.h"
#include "eventql/server/sql/partition_cursor.h"
#include "eventql/transport/native/frames/error.h"
#include "eventql/transport/native/frames/query_remote.h"
#include "eventql/transport/native/frames/query_remote_result.h"
#include "eventql/db/database.h"

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

ReturnCode PartitionCursor::execute() {
  return ReturnCode::success();
}

ReturnCode PartitionCursor::nextBatch(
    csql::SVector* columns,
    size_t* nrows) {
  for (;;) {
    if (cur_scan_.get() == nullptr) {
      if (!openNextTable()) {
        break;
      }
    }

    auto rc = cur_scan_->nextBatch(columns, nrows);
    if (!rc.isSuccess()) {
      return rc;
    }

    if (!*nrows) {
      cur_scan_.reset(nullptr);
      continue;
    }

    return ReturnCode::success();
  }

  *nrows = 0;
  return ReturnCode::success();
}

bool PartitionCursor::openNextTable() {
  RefPtr<cstable::CSTableReader> cstable;
  String cstable_filename;
  RefPtr<cstable::ColumnReader> skip_col;

  cur_skiplist_.reset();

  switch (cur_table_) {
    case 0: {
      if (snap_->head_arena.get() &&
          snap_->head_arena->getCSTableFile()) {
        cur_skiplist_.reset(
            new PartitionArena::SkiplistReader(
                snap_->head_arena->getSkiplistReader()));
        cstable = cstable::CSTableReader::openFile(
            snap_->head_arena->getCSTableFile(),
            cur_skiplist_->size());
        cstable_filename = StringUtil::format(
            "memory://$0/$1/$2/head_arena",
            snap_->state.tsdb_namespace(),
            snap_->state.table_key(),
            snap_->key.toString());
        break;
      } else {
        ++cur_table_;
        /* fallthrough */
      }
    }

    case 1: {
      if (snap_->compacting_arena.get() &&
          snap_->compacting_arena->getCSTableFile()) {
        cur_skiplist_.reset(
            new PartitionArena::SkiplistReader(
                snap_->compacting_arena->getSkiplistReader()));
        cstable = cstable::CSTableReader::openFile(
            snap_->compacting_arena->getCSTableFile(),
            cur_skiplist_->size());
        cstable_filename = StringUtil::format(
            "memory://$0/$1/$2/compacting_arena",
            snap_->state.tsdb_namespace(),
            snap_->state.table_key(),
            snap_->key.toString());
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
      const auto& tbl = (snap_->state.lsm_tables().data())[
          snap_->state.lsm_tables().size() - (cur_table_ - 1)];
      auto cstable_file = FileUtil::joinPaths(
          snap_->base_path,
          tbl->filename() + ".cst");
      cstable = cstable::CSTableReader::openFile(cstable_file);
      cstable_filename = cstable_file;
      if (tbl->has_skiplist()) {
        skip_col = cstable->getColumnReader("__lsm_skip");
      }
      break;
    }
  }

  auto id_col = cstable->getColumnReader("__lsm_id");
  auto is_update_col = cstable->getColumnReader("__lsm_is_update");
  cur_scan_.reset(
      new csql::FastCSTableScan(
          txn_,
          execution_context_,
          stmt_,
          cstable,
          cstable_filename));

  //cur_scan_->setFilter([this, id_col, is_update_col, skip_col] () -> bool {
  //  uint64_t rlvl;
  //  uint64_t dlvl;

  //  String id_str;
  //  id_col->readString(&rlvl, &dlvl, &id_str);
  //  SHA1Hash id(id_str.data(), id_str.size());

  //  bool is_update;
  //  is_update_col->readBoolean(&rlvl, &dlvl, &is_update);

  //  bool skip = false;
  //  if (skip_col.get()) {
  //    skip_col->readBoolean(&rlvl, &dlvl, &skip);
  //  }

  //  if (cur_skiplist_) {
  //    skip = cur_skiplist_->readNext();
  //  }

  //  if (skip || id_set_.count(id) > 0) {
  //    return false;
  //  } else {
  //    if (is_update) {
  //      id_set_.emplace(id);
  //    }
  //    return true;
  //  }
  //});

  //for (const auto& col : table_->schema()->columns()) {
  //  switch (col.second.type) {
  //    case msg::FieldType::BOOLEAN:
  //      cur_scan_->setColumnType(col.first, csql::SType::BOOL);
  //      break;
  //    case msg::FieldType::UINT32:
  //      cur_scan_->setColumnType(col.first, csql::SType::INT64);
  //      break;
  //    case msg::FieldType::UINT64:
  //      cur_scan_->setColumnType(col.first, csql::SType::INT64);
  //      break;
  //    case msg::FieldType::STRING:
  //      cur_scan_->setColumnType(col.first, csql::SType::STRING);
  //      break;
  //    case msg::FieldType::DOUBLE:
  //      cur_scan_->setColumnType(col.first, csql::SType::FLOAT64);
  //      break;
  //    case msg::FieldType::DATETIME:
  //      cur_scan_->setColumnType(col.first, csql::SType::TIMESTAMP64);
  //      break;
  //    case msg::FieldType::OBJECT:
  //      break;
  //  }
  //}

  ++cur_table_;

  auto rc = cur_scan_->execute();
  if (!rc.isSuccess()) {
    RAISE(kRuntimeError, rc.getMessage());
  }

  return true;
}

size_t PartitionCursor::getColumnCount() const {
  return stmt_->getNumComputedColumns();
}

csql::SType PartitionCursor::getColumnType(size_t idx) const {
  return stmt_->getColumnType(idx);
}

RemotePartitionCursor::RemotePartitionCursor(
    Session* session,
    csql::Transaction* txn,
    csql::ExecutionContext* execution_context,
    const std::string& database,
    RefPtr<csql::SequentialScanNode> stmt,
    const std::vector<std::string>& servers) :
    txn_(txn),
    execution_context_(execution_context),
    database_(database),
    stmt_(stmt),
    servers_(servers),
    ncols_(stmt->getNumComputedColumns()),
    row_buf_pos_(0),
    running_(false),
    done_(false),
    timeout_(
        session->getDatabaseContext()->config->getInt(
            "server.s2s_io_timeout").get()),
    client_(
        session->getDatabaseContext()->connection_pool,
        session->getDatabaseContext()->dns_cache,
        session->getDatabaseContext()->config->getInt(
            "server.s2s_io_timeout",
            0),
        session->getDatabaseContext()->config->getInt(
            "server.s2s_idle_timeout",
            0)) {}

ReturnCode RemotePartitionCursor::execute() {
  return ReturnCode::success();
}

/*
bool RemotePartitionCursor::next(csql::SValue* row, size_t row_len) {
  if (row_buf_pos_ == row_buf_.size() && !done_) {
    auto rc = fetchRows();
    if (!rc.isSuccess()) {
      RAISE(kRuntimeError, rc.getMessage());
    }
  }

  if (row_buf_pos_ == row_buf_.size()) {
    return false;
  } else {
    assert(row_buf_pos_ + ncols_ <= row_buf_.size());
    auto n = std::min(ncols_, (size_t) row_len);
    for (size_t i = 0; i < n; ++i) {
      row[i] = row_buf_[row_buf_pos_ + i];
    }

    row_buf_pos_ += ncols_;
    return true;
  }
}
*/

ReturnCode RemotePartitionCursor::nextBatch(
    csql::SVector* columns,
    size_t* nrecords) {
  return ReturnCode::error("ERUNTIME", "RemotePartitionCursor::nextBatch not yet implemented");
}

size_t RemotePartitionCursor::getColumnCount() const {
  return stmt_->getNumComputedColumns();
}

csql::SType RemotePartitionCursor::getColumnType(size_t idx) const {
  return stmt_->getColumnType(idx);
}

/*
ReturnCode RemotePartitionCursor::fetchRows() {
  if (running_) {
    auto rc = client_.sendFrame(EVQL_OP_QUERY_CONTINUE, 0, nullptr, 0);
    if (!rc.isSuccess()) {
      return rc;
    }
  } else {
    auto session = static_cast<eventql::Session*>(txn_->getUserData());
    auto cdir = session->getDatabaseContext()->config_directory;

    std::string qtree_coded;
    auto qtree_coded_os = StringOutputStream::fromString(&qtree_coded);
    csql::QueryTreeCoder qtree_coder(txn_);
    qtree_coder.encode(stmt_.get(), qtree_coded_os.get());

    native_transport::QueryRemoteFrame q_frame;
    q_frame.setEncodedQtree(std::move(qtree_coded));
    q_frame.setDatabase(session->getEffectiveNamespace());

    for (const auto& s : servers_) {
      auto server_cfg = cdir->getServerConfig(s);
      if (server_cfg.server_status() != SERVER_UP) {
        logError("evqld", "server is down: $0", s);
        continue;
      }

      auto rc = client_.connect(server_cfg.server_addr(), true);
      if (!rc.isSuccess()) {
        logError("evqld", "Remote SQL Error: $0", rc.getMessage());
        continue;
      }

      rc = client_.sendFrame(&q_frame, 0);
      if (rc.isSuccess()) {
        running_ = true;
      } else {
        logError("evqld", "Remote SQL Error: $0", rc.getMessage());
        continue;
      }
    }

    if (!running_) {
      return ReturnCode::error("ERUNTIME", "no server available for query");
    }
  }

  uint16_t ret_opcode = 0;
  uint16_t ret_flags;
  std::string ret_payload;
  while (ret_opcode != EVQL_OP_QUERY_REMOTE_RESULT) {
    auto rc = txn_->triggerHeartbeat();
    if (!rc.isSuccess()) {
      return rc;
    }

    rc = client_.recvFrame(&ret_opcode, &ret_flags, &ret_payload, timeout_);
    if (!rc.isSuccess()) {
      return rc;
    }

    if (ret_flags & EVQL_ENDOFREQUEST) {
      done_ = true;
    }

    switch (ret_opcode) {
      case EVQL_OP_HEARTBEAT:
        continue;
      case EVQL_OP_QUERY_REMOTE_RESULT:
        break;
      case EVQL_OP_ERROR: {
        native_transport::ErrorFrame eframe;
        eframe.parseFrom(ret_payload.data(), ret_payload.size());
        return ReturnCode::error("ERUNTIME", eframe.getError());
      }
      default:
        return ReturnCode::error("ERUNTIME", "invalid opcode");
    }
  }

  native_transport::QueryRemoteResultFrame r_frame;
  auto rc = r_frame.parseFrom(ret_payload.data(), ret_payload.size());

  if (r_frame.getColumnCount() != ncols_) {
    return ReturnCode::error("ERUNTIME", "invalid column count");
  }

  auto is = r_frame.getRowDataInputStream();
  auto n = r_frame.getRowCount() * ncols_;
  row_buf_.clear();
  row_buf_.resize(n);
  row_buf_pos_ = 0;
  try {
    for (size_t i = 0; i < n; ++i) {
        row_buf_[i].decode(is.get());
    }
  } catch (const std::exception& e) {
    return ReturnCode::error("ERUNTIME", e.what());
  }

  return ReturnCode::success();
}
*/

}
