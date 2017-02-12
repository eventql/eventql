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
#include <eventql/sql/statements/select/select.h>

namespace csql {

SelectExpression::SelectExpression(
    Transaction* txn,
    ExecutionContext* execution_context,
    Vector<ValueExpression> select_expressions) :
    txn_(txn),
    execution_context_(execution_context),
    select_exprs_(std::move(select_expressions)),
    pos_(0) {
  execution_context_->incrementNumTasks();
}

ReturnCode SelectExpression::execute() {
  execution_context_->incrementNumTasksRunning();

  return ReturnCode::success();
}

ReturnCode SelectExpression::nextBatch(
    SVector* columns,
    size_t* nrecords) {
  if (pos_++ == 0) {
    for (int i = 0; i < select_exprs_.size(); ++i) {
      VM::evaluate(
          txn_,
          select_exprs_[i].program(),
          select_exprs_[i].program()->method_call,
          &vm_stack_,
          nullptr,
          0,
          nullptr);

      popVector(&vm_stack_, &columns[i]);
    }

    execution_context_->incrementNumTasksCompleted();
    *nrecords = 1;
  } else {
    *nrecords = 0;
  }

  return ReturnCode::success();
}

size_t SelectExpression::getColumnCount() const {
  return select_exprs_.size();
}

SType SelectExpression::getColumnType(size_t idx) const {
  assert(idx < select_exprs_.size());
  return select_exprs_[idx].getReturnType();
}

} // namespace csql

