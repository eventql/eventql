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
#include <csql/runtime/TableExpression.h>
#include <csql/runtime/defaultruntime.h>

namespace csql {

class SelectExpression : public TableExpression {
public:

  SelectExpression(
      Transaction* ctx,
      const Vector<String>& column_names,
      Vector<ValueExpression> select_expressions);

  void prepare(ExecutionContext* context) override;

  void execute(
      ExecutionContext* context,
      Function<bool (int argc, const SValue* argv)> fn) override;

  Vector<String> columnNames() const override;

  size_t numColumns() const override;

protected:
  Transaction* ctx_;
  Vector<String> column_names_;
  Vector<ValueExpression> select_exprs_;
};

}
