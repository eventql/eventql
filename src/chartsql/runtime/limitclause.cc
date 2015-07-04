/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/runtime/limitclause.h>

namespace csql {

LimitClause::LimitClause(
    int limit,
    int offset,
    ScopedPtr<TableExpression> child) :
    limit_(limit),
    offset_(offset),
    child_(std::move(child)) {}


void LimitClause::execute(
    ExecutionContext* context,
    Function<bool (int argc, const SValue* argv)> fn) {
  size_t counter = 0;

  child_->execute(
      context,
      [this, &counter, &fn] (int argc, const SValue* argv) -> bool {
    if (counter++ < offset_) {
      return true;
    }

    if (counter > (offset_ + limit_)) {
      return false;
    }

    return fn(argc, argv);
  });
}

Vector<String> LimitClause::columnNames() const {
  return child_->columnNames();
}

size_t LimitClause::numColunns() const {
  return child_->numColunns();
}

}
