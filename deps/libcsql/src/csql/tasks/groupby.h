/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/SHA1.h>
#include <csql/tasks/Task.h>
#include <csql/runtime/defaultruntime.h>

namespace csql {

class GroupBy : public Task {
public:

  GroupBy(
      Transaction* txn,
      Vector<ValueExpression> select_expressions,
      Vector<ValueExpression> group_expressions,
      HashMap<TaskID, ScopedPtr<ResultCursor>> input);

  bool nextRow(SValue* out, int out_len) override;

  //bool onInputRow(
  //    const TaskID& input_id,
  //    const SValue* row,
  //    int row_len) override;

  //void onInputsReady() override;

protected:

  void freeResult();

  Transaction* txn_;
  Vector<ValueExpression> select_exprs_;
  Vector<ValueExpression> group_exprs_;
  ScopedPtr<ResultCursorList> input_;
  HashMap<String, Vector<VM::Instance>> groups_;
  ScratchMemory scratch_;
};

class GroupByFactory : public TaskFactory {
public:

  GroupByFactory(
      Vector<RefPtr<SelectListNode>> select_exprs,
      Vector<RefPtr<ValueExpressionNode>> group_exprs);

  RefPtr<Task> build(
      Transaction* txn,
      HashMap<TaskID, ScopedPtr<ResultCursor>> input) const override;

protected:
  Vector<RefPtr<SelectListNode>> select_exprs_;
  Vector<RefPtr<ValueExpressionNode>> group_exprs_;
};

}
