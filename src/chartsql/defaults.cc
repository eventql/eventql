/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <chartsql/defaults.h>

using namespace fnord;

namespace csql {

void installDefaultSymbols(SymbolTable* symbol_table) {
  /* expressions/aggregate.h */
  symbol_table->registerFunction("count", expressions::kCountExpr);
  symbol_table->registerFunction("sum", expressions::kSumExpr);

  symbol_table->registerSymbol(
      "mean",
      &expressions::meanExpr,
      expressions::meanExprScratchpadSize(),
      &expressions::meanExprFree);

  symbol_table->registerSymbol(
      "avg",
      &expressions::meanExpr,
      expressions::meanExprScratchpadSize(),
      &expressions::meanExprFree);

  symbol_table->registerSymbol(
      "average",
      &expressions::meanExpr,
      expressions::meanExprScratchpadSize(),
      &expressions::meanExprFree);

  symbol_table->registerSymbol(
      "min",
      &expressions::minExpr,
      expressions::minExprScratchpadSize(),
      &expressions::minExprFree);

  symbol_table->registerSymbol(
      "max",
      &expressions::maxExpr,
      expressions::maxExprScratchpadSize(),
      &expressions::maxExprFree);

  /* expressions/boolean.h */
  symbol_table->registerFunction("eq", &expressions::eqExpr);
  symbol_table->registerFunction("neq", &expressions::neqExpr);
  symbol_table->registerFunction("and", &expressions::andExpr);
  symbol_table->registerFunction("or", &expressions::orExpr);
  symbol_table->registerFunction("neg", &expressions::negExpr);
  symbol_table->registerFunction("lt", &expressions::ltExpr);
  symbol_table->registerFunction("lte", &expressions::lteExpr);
  symbol_table->registerFunction("gt", &expressions::gtExpr);
  symbol_table->registerFunction("gte", &expressions::gteExpr);

  /* expressions/datetime.h */
  symbol_table->registerSymbol("FROM_TIMESTAMP", &expressions::fromTimestamp);

  /* expressions/math.h */
  symbol_table->registerFunction("add", &expressions::addExpr);
  symbol_table->registerFunction("sub", &expressions::subExpr);
  symbol_table->registerFunction("mul", &expressions::mulExpr);
  symbol_table->registerFunction("div", &expressions::divExpr);
  symbol_table->registerFunction("mod", &expressions::modExpr);
  symbol_table->registerFunction("pow", &expressions::powExpr);

}

} // namespace csql
