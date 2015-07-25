/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <stx/stdtypes.h>
#include <chartsql/runtime/TableExpression.h>
#include <chartsql/runtime/queryplan.h>
#include <chartsql/svalue.h>

using namespace stx;

namespace csql {

class DefaultExecutionPlan : public ExecutionPlan {
public:

  DefaultExecutionPlan(ScopedPtr<TableExpression> entry);

  void execute(Function<bool (int argc, const SValue* argv)> fn) override;

protected:
  ScopedPtr<TableExpression> entry_;
  ExecutionContext context_;
};

} // namespace csql
