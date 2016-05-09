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
#include <eventql/sql/tasks/Task.h>
#include <eventql/sql/runtime/defaultruntime.h>

namespace csql {

class Select : public TableExpression {
public:

  Select(
      Transaction* txn,
      Vector<ValueExpression> select_expressions);

  ScopedPtr<ResultCursor> execute() override;

  //void onInputsReady() override;

protected:
  Transaction* txn_;
  Vector<ValueExpression> select_exprs_;
  size_t pos_;
};

}
