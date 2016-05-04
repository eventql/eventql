/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2011-2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <algorithm>
#include <csql/runtime/orderby.h>
#include <csql/expressions/boolean.h>
#include <csql/runtime/runtime.h>
#include <stx/inspect.h>

namespace csql {

OrderBy::OrderBy(
    Transaction* ctx,
    Vector<SortExpr> sort_specs,
    ScopedPtr<TableExpression> child) :
    ctx_(ctx),
    sort_specs_(std::move(sort_specs)),
    child_(std::move(child)) {
  if (sort_specs_.size() == 0) {
    RAISE(kIllegalArgumentError, "can't execute ORDER BY: no sort specs");
  }
}

void OrderBy::prepare(ExecutionContext* context) {
  context->incrNumSubtasksTotal(1);
  child_->prepare(context);
}

// FIXPAUL this should mergesort while inserting...
void OrderBy::execute(
    ExecutionContext* context,
    Function<bool (int argc, const SValue* argv)> fn) {
  Vector<Vector<SValue>> rows;

  child_->execute(context, [&rows] (int argc, const SValue* argv) -> bool {
    Vector<SValue> row;
    for (int i = 0; i < argc; i++) {
      row.emplace_back(argv[i]);
    }

    rows.emplace_back(row);
    return true;
  });

  auto rt = ctx_->getRuntime();
  std::sort(
      rows.begin(),
      rows.end(),
      [this, rt] (const Vector<SValue>& left, const Vector<SValue>& right) -> bool {
    for (const auto& sort : sort_specs_) {
      SValue args[2];
      SValue res(false);
      args[0] = rt->evaluateScalarExpression(
          ctx_,
          sort.expr,
          left.size(),
          left.data());
      args[1] = rt->evaluateScalarExpression(
          ctx_,
          sort.expr,
          right.size(),
          right.data());

      expressions::eqExpr(Transaction::get(ctx_), 2, args, &res);
      if (res.getBool()) {
        continue;
      }

      if (sort.descending) {
        expressions::gtExpr(Transaction::get(ctx_), 2, args, &res);
      } else {
        expressions::ltExpr(Transaction::get(ctx_), 2, args, &res);
      }

      return res.getBool();
    }

    /* all dimensions equal */
    return false;
  });

  context->incrNumSubtasksCompleted(1);

  for (auto& row : rows) {
    if (!fn(row.size(), row.data())) {
      return;
    }
  }
}

Vector<String> OrderBy::columnNames() const {
  return child_->columnNames();
}

size_t OrderBy::numColumns() const {
  return child_->numColumns();
}

} // namespace csql
