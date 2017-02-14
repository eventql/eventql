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
#include <eventql/sql/statements/select/nested_loop_join.h>

namespace csql {

NestedLoopJoin::NestedLoopJoin(
    Transaction* txn,
    JoinType join_type,
    const Vector<JoinNode::InputColumnRef>& input_map,
    Vector<ValueExpression> select_expressions,
    Option<ValueExpression> join_cond_expr,
    Option<ValueExpression> where_expr,
    ScopedPtr<TableExpression> base_tbl,
    ScopedPtr<TableExpression> joined_tbl) :
    txn_(txn),
    join_type_(join_type),
    input_map_(input_map),
    select_exprs_(std::move(select_expressions)),
    join_cond_expr_(std::move(join_cond_expr)),
    where_expr_(std::move(where_expr)),
    base_tbl_(std::move(base_tbl)),
    joined_tbl_(std::move(joined_tbl)),
    joined_tbl_size_(0),
    joined_tbl_pos_(0),
    joined_tbl_row_found_(false),
    base_tbl_buffer_size_(0) {}

static const size_t kMaxInMemoryRows = 1000000;

ReturnCode NestedLoopJoin::execute() {
  /* read joined table into hash map */
  {
    auto rc = readJoinedTable();
    if (!rc.isSuccess()) {
      return rc;
    }
  }

  /* start base table execution */
  {
    auto rc = base_tbl_->execute();
    if (!rc.isSuccess()) {
      return rc;
    }
  }

  for (size_t i = 0; i < base_tbl_->getColumnCount(); ++i) {
    base_tbl_buffer_.emplace_back(base_tbl_->getColumnType(i));
    base_tbl_cur_.emplace_back(nullptr);
  }

  return ReturnCode::success();
}

size_t NestedLoopJoin::getColumnCount() const {
  return select_exprs_.size();
}

SType NestedLoopJoin::getColumnType(size_t idx) const {
  assert(idx < select_exprs_.size());
  return select_exprs_[idx].getReturnType();
}

ReturnCode NestedLoopJoin::nextBatch(
    SVector* columns,
    size_t* nrecords) {
  *nrecords = 0;

  switch (join_type_) {
    case JoinType::OUTER:
      return executeOuterJoin(columns, nrecords);
    case JoinType::INNER:
      if (join_cond_expr_.isEmpty()) {
        /* fallthrough */
      } else {
        return executeInnerJoin(columns, nrecords);
      }
    case JoinType::CARTESIAN:
      return executeCartesianJoin(columns, nrecords);
    default:
      return ReturnCode::error("EARG", "invalid join type");
  }
}

ReturnCode NestedLoopJoin::executeCartesianJoin(
    SVector* output,
    size_t* output_len) {
  std::vector<const void*> input_buf(input_map_.size());

  if (joined_tbl_size_ == 0) {
    return ReturnCode::success();
  }

  for (;;) {
    if (joined_tbl_pos_ == joined_tbl_size_) {
      --base_tbl_buffer_size_;
      for (size_t i = 0; i < base_tbl_cur_.size(); ++i) {
        base_tbl_buffer_[i].next(const_cast<void**>(&base_tbl_cur_[i]));
      }

      joined_tbl_pos_ = 0;
      for (size_t i = 0; i < joined_tbl_->getColumnCount(); ++i) {
        joined_tbl_cur_[i] = joined_tbl_data_[i].getData();
        assert(joined_tbl_cur_[i]);
      }
    }

    if (base_tbl_buffer_size_ == 0) {
      auto rc = readBaseTableBatch();
      if (!rc.isSuccess()) {
        return rc;
      }
    }

    if (base_tbl_buffer_size_ == 0) {
      break;
    }

    while (joined_tbl_pos_ < joined_tbl_size_) {
      for (size_t i = 0; i < input_map_.size(); ++i) {
        const auto& m = input_map_[i];

        switch (m.table_idx) {
          case 0:
            assert(m.column_idx < base_tbl_cur_.size());
            input_buf[i] = base_tbl_cur_[m.column_idx];
            break;
          case 1:
            assert(m.column_idx < joined_tbl_cur_.size());
            input_buf[i] = joined_tbl_cur_[m.column_idx];
            break;
          default:
            RAISE(kRuntimeError, "invalid table index");
        }
      }

      ++joined_tbl_pos_;
      for (size_t i = 0; i < joined_tbl_cur_.size(); ++i) {
        joined_tbl_data_[i].next(const_cast<void**>(&joined_tbl_cur_[i]));
      }

      if (!where_expr_.isEmpty()) {
        VM::evaluate(
            txn_,
            where_expr_.get().program(),
            where_expr_.get().program()->method_call,
            &vm_stack_,
            nullptr,
            input_buf.size(),
            const_cast<void**>(input_buf.data()));

        if (!popBool(&vm_stack_)) {
          continue;
        }
      }

      for (int i = 0; i < select_exprs_.size(); ++i) {
        VM::evaluate(
            txn_,
            select_exprs_[i].program(),
            select_exprs_[i].program()->method_call,
            &vm_stack_,
            nullptr,
            input_buf.size(),
            const_cast<void**>(input_buf.data()));

        popVector(&vm_stack_, &output[i]);
      }

      if (++(*output_len) >= kOutputBatchSize) {
        return ReturnCode::success();
      }
    }
  }

  return ReturnCode::success();
}

ReturnCode NestedLoopJoin::executeInnerJoin(
    SVector* output,
    size_t* output_len) {
  std::vector<const void*> input_buf(input_map_.size());

  if (joined_tbl_size_ == 0) {
    return ReturnCode::success();
  }

  for (;;) {
    if (joined_tbl_pos_ == joined_tbl_size_) {
      --base_tbl_buffer_size_;
      for (size_t i = 0; i < base_tbl_cur_.size(); ++i) {
        base_tbl_buffer_[i].next(const_cast<void**>(&base_tbl_cur_[i]));
      }

      joined_tbl_pos_ = 0;
      for (size_t i = 0; i < joined_tbl_->getColumnCount(); ++i) {
        joined_tbl_cur_[i] = joined_tbl_data_[i].getData();
        assert(joined_tbl_cur_[i]);
      }
    }

    if (base_tbl_buffer_size_ == 0) {
      auto rc = readBaseTableBatch();
      if (!rc.isSuccess()) {
        return rc;
      }
    }

    if (base_tbl_buffer_size_ == 0) {
      break;
    }

    while (joined_tbl_pos_ < joined_tbl_size_) {
      for (size_t i = 0; i < input_map_.size(); ++i) {
        const auto& m = input_map_[i];

        switch (m.table_idx) {
          case 0:
            assert(m.column_idx < base_tbl_cur_.size());
            input_buf[i] = base_tbl_cur_[m.column_idx];
            break;
          case 1:
            assert(m.column_idx < joined_tbl_cur_.size());
            input_buf[i] = joined_tbl_cur_[m.column_idx];
            break;
          default:
            RAISE(kRuntimeError, "invalid table index");
        }
      }

      ++joined_tbl_pos_;
      for (size_t i = 0; i < joined_tbl_cur_.size(); ++i) {
        joined_tbl_data_[i].next(const_cast<void**>(&joined_tbl_cur_[i]));
      }

      if (!where_expr_.isEmpty()) {
        VM::evaluate(
            txn_,
            where_expr_.get().program(),
            where_expr_.get().program()->method_call,
            &vm_stack_,
            nullptr,
            input_buf.size(),
            const_cast<void**>(input_buf.data()));

        if (!popBool(&vm_stack_)) {
          continue;
        }
      }

      VM::evaluate(
          txn_,
          join_cond_expr_.get().program(),
          join_cond_expr_.get().program()->method_call,
          &vm_stack_,
          nullptr,
          input_buf.size(),
          const_cast<void**>(input_buf.data()));

      if (!popBool(&vm_stack_)) {
        continue;
      }

      for (int i = 0; i < select_exprs_.size(); ++i) {
        VM::evaluate(
            txn_,
            select_exprs_[i].program(),
            select_exprs_[i].program()->method_call,
            &vm_stack_,
            nullptr,
            input_buf.size(),
            const_cast<void**>(input_buf.data()));

        popVector(&vm_stack_, &output[i]);
      }

      if (++(*output_len) >= kOutputBatchSize) {
        return ReturnCode::success();
      }
    }
  }

  return ReturnCode::success();
}

ReturnCode NestedLoopJoin::executeOuterJoin(
    SVector* output,
    size_t* output_len) {
  std::vector<const void*> input_buf(input_map_.size());
  uint64_t null = 0;

  if (joined_tbl_size_ == 0) {
    return ReturnCode::success();
  }

  for (;;) {
    if (joined_tbl_pos_ == joined_tbl_size_) {
      --base_tbl_buffer_size_;
      for (size_t i = 0; i < base_tbl_cur_.size(); ++i) {
        base_tbl_buffer_[i].next(const_cast<void**>(&base_tbl_cur_[i]));
      }

      joined_tbl_pos_ = 0;
      joined_tbl_row_found_ = false;
      for (size_t i = 0; i < joined_tbl_->getColumnCount(); ++i) {
        joined_tbl_cur_[i] = joined_tbl_data_[i].getData();
        assert(joined_tbl_cur_[i]);
      }
    }

    if (base_tbl_buffer_size_ == 0) {
      auto rc = readBaseTableBatch();
      if (!rc.isSuccess()) {
        return rc;
      }
    }

    if (base_tbl_buffer_size_ == 0) {
      break;
    }

    bool match = false;
    while (joined_tbl_pos_ < joined_tbl_size_) {
      for (size_t i = 0; i < input_map_.size(); ++i) {
        const auto& m = input_map_[i];

        switch (m.table_idx) {
          case 0:
            assert(m.column_idx < base_tbl_cur_.size());
            input_buf[i] = base_tbl_cur_[m.column_idx];
            break;
          case 1:
            assert(m.column_idx < joined_tbl_cur_.size());
            input_buf[i] = joined_tbl_cur_[m.column_idx];
            break;
          default:
            RAISE(kRuntimeError, "invalid table index");
        }
      }

      ++joined_tbl_pos_;
      for (size_t i = 0; i < joined_tbl_cur_.size(); ++i) {
        joined_tbl_data_[i].next(const_cast<void**>(&joined_tbl_cur_[i]));
      }

      VM::evaluate(
          txn_,
          join_cond_expr_.get().program(),
          join_cond_expr_.get().program()->method_call,
          &vm_stack_,
          nullptr,
          input_buf.size(),
          const_cast<void**>(input_buf.data()));

      if (!popBool(&vm_stack_)) {
        continue;
      }

      joined_tbl_row_found_ = true;
      match = true;
      break;
    }

    if (match || !joined_tbl_row_found_) {
      if (!joined_tbl_row_found_) {
        for (size_t i = 0; i < input_map_.size(); ++i) {
          const auto& m = input_map_[i];

          switch (m.table_idx) {
            case 0:
              assert(m.column_idx < base_tbl_cur_.size());
              input_buf[i] = base_tbl_cur_[m.column_idx];
              break;
            case 1:
              assert(m.column_idx < joined_tbl_cur_.size());
              input_buf[i] = &null;
              break;
            default:
              RAISE(kRuntimeError, "invalid table index");
          }
        }
      }

      if (!where_expr_.isEmpty()) {
        VM::evaluate(
            txn_,
            where_expr_.get().program(),
            where_expr_.get().program()->method_call,
            &vm_stack_,
            nullptr,
            input_buf.size(),
            const_cast<void**>(input_buf.data()));

        if (!popBool(&vm_stack_)) {
          continue;
        }
      }

      for (int i = 0; i < select_exprs_.size(); ++i) {
        VM::evaluate(
            txn_,
            select_exprs_[i].program(),
            select_exprs_[i].program()->method_call,
            &vm_stack_,
            nullptr,
            input_buf.size(),
            const_cast<void**>(input_buf.data()));

        popVector(&vm_stack_, &output[i]);
      }

      if (++(*output_len) >= kOutputBatchSize) {
        return ReturnCode::success();
      }
    }
  }

  return ReturnCode::success();
}

ReturnCode NestedLoopJoin::readJoinedTable() {
  auto joined_rc = joined_tbl_->execute();
  if (!joined_rc.isSuccess()) {
    return joined_rc;
  }

  Vector<SVector> input_cols;
  for (size_t i = 0; i < joined_tbl_->getColumnCount(); ++i) {
    input_cols.emplace_back(joined_tbl_->getColumnType(i));
    joined_tbl_data_.emplace_back(joined_tbl_->getColumnType(i));
  }

  for (;;) {
    for (auto& c : input_cols) {
      c.clear();
    }

    size_t nrecords = 0;
    {
      auto rc = joined_tbl_->nextBatch(input_cols.data(), &nrecords);
      if (!rc.isSuccess()) {
        RAISE(kRuntimeError, rc.getMessage());
      }
    }

    if (nrecords == 0) {
      break;
    }

    {
      auto rc = txn_->triggerHeartbeat();
      if (!rc.isSuccess()) {
        RAISE(kRuntimeError, rc.getMessage());
      }
    }

    if (joined_tbl_size_ + nrecords >= kMaxInMemoryRows) {
      RAISE(
          kRuntimeError,
          "Nested Loop JOIN intermediate result set is too large, try using an"
          " equi-join instead.");
    }

    for (size_t i = 0; i < input_cols.size(); ++i) {
      joined_tbl_data_[i].append(
          input_cols[i].getData(),
          input_cols[i].getSize());
    }

    joined_tbl_size_ += nrecords;
  }

  for (size_t i = 0; i < joined_tbl_->getColumnCount(); ++i) {
    joined_tbl_cur_.emplace_back(joined_tbl_data_[i].getData());
    assert(joined_tbl_cur_[i]);
  }

  return ReturnCode::success();
}

ReturnCode NestedLoopJoin::readBaseTableBatch() {
  for (auto& c : base_tbl_buffer_) {
    c.clear();
  }

  {
    auto rc = base_tbl_->nextBatch(
        base_tbl_buffer_.data(),
        &base_tbl_buffer_size_);

    if (!rc.isSuccess()) {
      RAISE(kRuntimeError, rc.getMessage());
    }
  }

  if (base_tbl_buffer_size_ == 0) {
    return ReturnCode::success();
  }

  {
    auto rc = txn_->triggerHeartbeat();
    if (!rc.isSuccess()) {
      RAISE(kRuntimeError, rc.getMessage());
    }
  }

  for (size_t i = 0; i < base_tbl_->getColumnCount(); ++i) {
    base_tbl_cur_[i] = base_tbl_buffer_[i].getData();
    assert(base_tbl_cur_[i]);
  }

  return ReturnCode::success();
}

} // namespace csql

