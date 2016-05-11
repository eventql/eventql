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
#include <eventql/util/stdtypes.h>
#include <eventql/util/SHA1.h>
#include <eventql/sql/runtime/defaultruntime.h>

namespace csql {

class GroupByExpression : public TableExpression {
public:

  GroupByExpression(
      Transaction* txn,
      Vector<ValueExpression> select_expressions,
      Vector<ValueExpression> group_expressions,
      ScopedPtr<TableExpression> input);

  ~GroupByExpression();


  ScopedPtr<ResultCursor> execute() override;

  //bool onInputRow(
  //    const TaskID& input_id,
  //    const SValue* row,
  //    int row_len) override;

  //void onInputsReady() override;

protected:
  bool next(SValue* row, size_t row_len);

  void freeResult();

  Transaction* txn_;
  Vector<ValueExpression> select_exprs_;
  Vector<ValueExpression> group_exprs_;
  ScopedPtr<TableExpression> input_;
  HashMap<String, Vector<VM::Instance>> groups_;
  HashMap<String, Vector<VM::Instance>>::const_iterator groups_iter_;
  ScratchMemory scratch_;
  bool freed_;
};

}
