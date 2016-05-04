/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/runtime/SubqueryExpression.h>
#include <csql/runtime/vm.h>

namespace csql {

SubqueryExpression::SubqueryExpression(
    Transaction* txn,
    const Vector<String>& column_names,
    Vector<ValueExpression> select_expressions,
    Option<ValueExpression> where_expr,
    ScopedPtr<TableExpression> subquery) :
    txn_(txn),
    column_names_(column_names),
    select_exprs_(std::move(select_expressions)),
    where_expr_(std::move(where_expr)),
    subquery_(std::move(subquery)) {}

void SubqueryExpression::prepare(ExecutionContext* context) {
  context->incrNumSubtasksTotal(1);
  subquery_->prepare(context);
}

void SubqueryExpression::execute(
    ExecutionContext* ctx,
    Function<bool (int argc, const SValue* argv)> fn) {
  Vector<SValue> buf(select_exprs_.size(), SValue{});

  subquery_->execute(ctx, [this, &fn, &buf] (int inc, const SValue* inv) {
    if (!where_expr_.isEmpty()) {
      SValue pred;
      VM::evaluate(txn_, where_expr_.get().program(), inc, inv, &pred);
      if (!pred.getBool()) {
        return true;
      }
    }

    for (int i = 0; i < select_exprs_.size(); ++i) {
      VM::evaluate(txn_, select_exprs_[i].program(), inc, inv, &buf[i]);
    }

    return fn(buf.size(), buf.data());
  });

  ctx->incrNumSubtasksCompleted(1);
}

Vector<String> SubqueryExpression::columnNames() const {
  return column_names_;
}

size_t SubqueryExpression::numColumns() const {
  return column_names_.size();
}

}
