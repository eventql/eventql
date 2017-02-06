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
#include <algorithm>
#include <array>
#include <eventql/sql/statements/select/orderby.h>
#include <eventql/sql/expressions/boolean.h>
#include <eventql/sql/runtime/runtime.h>
#include <eventql/util/inspect.h>

namespace csql {

OrderByExpression::OrderByExpression(
    Transaction* txn,
    ExecutionContext* execution_context,
    Vector<SortExpr> sort_specs,
    ScopedPtr<TableExpression> input) :
    txn_(txn),
    execution_context_(execution_context),
    sort_specs_(std::move(sort_specs)),
    input_(std::move(input)),
    num_rows_(0),
    pos_(0),
    cnt_(0) {
  if (sort_specs_.size() == 0) {
    RAISE(kIllegalArgumentError, "can't execute ORDER BY: no sort specs");
  }

  execution_context_->incrementNumTasks();
}

ReturnCode OrderByExpression::execute() {
  auto rc = input_->execute();
  if (!rc.isSuccess()) {
    return rc;
  }

  execution_context_->incrementNumTasksRunning();

  Vector<SVector> input_cols;
  for (size_t i = 0; i < input_->getColumnCount(); ++i) {
    input_cols.emplace_back(input_->getColumnType(i));
  }

  for (;;) {
    for (auto& c : input_cols) {
      c.clear();
    }

    size_t nrecords = 0;
    {
      auto rc = input_->nextBatch(input_cols.data(), &nrecords);
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

    Vector<void*> input_col_cursor;
    for (const auto& c : input_cols) {
      input_col_cursor.emplace_back((void*) c.getData());
    }


    for (size_t n = 0; n < nrecords; n++) {
      rows_.emplace_back();
      auto& row = rows_.back();

      for (size_t i = 0; i < input_->getColumnCount(); ++i) {
        row.emplace_back(input_->getColumnType(i));
        row[i].copyFrom(input_col_cursor[i]);
      }
    }
  }

  num_rows_ = rows_.size();

  std::sort(
      rows_.begin(),
      rows_.end(),
      [this] (const Vector<SValue>& left, const Vector<SValue>& right) -> bool {
    if (++cnt_ % 4096 == 0) {
      auto rc = txn_->triggerHeartbeat();
      if (!rc.isSuccess()) {
        RAISE(kRuntimeError, rc.getMessage());
      }
    }

    for (const auto& sort : sort_specs_) {
      VM::evaluateBoxed(
          txn_,
          sort.expr.program(),
          sort.expr.program()->method_call,
          &vm_stack_,
          nullptr,
          left.size(),
          left.data());

      VM::evaluateBoxed(
          txn_,
          sort.expr.program(),
          sort.expr.program()->method_call,
          &vm_stack_,
          nullptr,
          right.size(),
          right.data());

      std::array<SValue, 2> args = {
          sort.expr.program()->return_type,
          sort.expr.program()->return_type };

      popBoxed(&vm_stack_, &args[0]);
      popBoxed(&vm_stack_, &args[1]);

      SValue res(false);
      expressions::eqExpr(Transaction::get(txn_), 2, args.data(), &res);
      if (res.getBool()) {
        continue;
      }

      if (sort.descending) {
        expressions::gtExpr(Transaction::get(txn_), 2, args.data(), &res);
      } else {
        expressions::ltExpr(Transaction::get(txn_), 2, args.data(), &res);
      }

      return res.getBool();
    }

    /* all dimensions equal */
    return false;
  });

  return ReturnCode::success();
}

ReturnCode OrderByExpression::nextBatch(
    SVector* output,
    size_t* nrecords) {
  *nrecords = 0;

  if (pos_ < num_rows_) {
    auto batch_len = std::min(num_rows_ - pos_, kOutputBatchSize);
    auto ncols = input_->getColumnCount();

    for (size_t n = 0; n < batch_len; ++n) {
      for (size_t i = 0; i < ncols; ++i) {
        output[i].append(
            rows_[pos_][i].getData(),
            rows_[pos_][i].getSize());
      }
    }

    *nrecords += batch_len;
    pos_ += batch_len;
    if (pos_ == num_rows_) {
      execution_context_->incrementNumTasksCompleted();
    }
  }

  return ReturnCode::success();
}

size_t OrderByExpression::getColumnCount() const {
  return input_->getColumnCount();
}

SType OrderByExpression::getColumnType(size_t idx) const {
  return input_->getColumnType(idx);
}

} // namespace csql
