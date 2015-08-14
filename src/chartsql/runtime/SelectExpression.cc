/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/runtime/SelectExpression.h>

namespace csql {

SelectExpression::SelectExpression(
    const Vector<String>& column_names,
    Vector<ValueExpression> select_expressions) :
    column_names_(column_names),
    select_exprs_(std::move(select_expressions)) {}

void SelectExpression::execute(
    ExecutionContext* context,
    Function<bool (int argc, const SValue* argv)> fn) {
  Vector<SValue> out_row(select_exprs_.size(), SValue{});

  for (int i = 0; i < select_exprs_.size(); ++i) {
    VM::evaluate(select_exprs_[i].program(), 0, nullptr,  &out_row[i]);
  }

  fn(out_row.size(), out_row.data());
}

Vector<String> SelectExpression::columnNames() const {
  return column_names_;
}

size_t SelectExpression::numColumns() const {
  return column_names_.size();
}

}
