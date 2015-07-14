/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORDMETRIC_SQL_RUNTIME_H
#define _FNORDMETRIC_SQL_RUNTIME_H
#include <stdlib.h>
#include <string>
#include <vector>
#include <memory>
#include <chartsql/parser/parser.h>
#include <chartsql/runtime/queryplan.h>
#include <chartsql/runtime/queryplanbuilder.h>
#include <chartsql/runtime/QueryBuilder.h>
#include <chartsql/runtime/symboltable.h>
#include <chartsql/runtime/ResultFormat.h>
#include <chartsql/runtime/ExecutionStrategy.h>

namespace csql {

class Runtime {
public:

  Runtime();

  void executeQuery(
      const String& query,
      RefPtr<ExecutionStrategy> execution_strategy,
      RefPtr<ResultFormat> result_format);

  SValue evaluateStaticExpression(const String& expr);
  SValue evaluateStaticExpression(ASTNode* expr);

  void registerFunction(const String& name, SFunction fn);

protected:
  SymbolTable symbol_table_;
  QueryBuilder query_builder_;
  QueryPlanBuilder query_plan_builder_;
};

}
#endif
