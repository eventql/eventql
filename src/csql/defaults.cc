/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <csql/defaults.h>

using namespace stx;

namespace csql {

void installDefaultSymbols(SymbolTable* rt) {
  /* expressions/aggregate.h */
  rt->registerFunction("count", expressions::kCountExpr);
  rt->registerFunction("sum", expressions::kSumExpr);

  //rt->registerSymbol(
  //    "mean",
  //    &expressions::meanExpr,
  //    expressions::meanExprScratchpadSize(),
  //    &expressions::meanExprFree);

  //rt->registerSymbol(
  //    "avg",
  //    &expressions::meanExpr,
  //    expressions::meanExprScratchpadSize(),
  //    &expressions::meanExprFree);

  //rt->registerSymbol(
  //    "average",
  //    &expressions::meanExpr,
  //    expressions::meanExprScratchpadSize(),
  //    &expressions::meanExprFree);

  //rt->registerSymbol(
  //    "min",
  //    &expressions::minExpr,
  //    expressions::minExprScratchpadSize(),
  //    &expressions::minExprFree);

  //rt->registerSymbol(
  //    "max",
  //    &expressions::maxExpr,
  //    expressions::maxExprScratchpadSize(),
  //    &expressions::maxExprFree);

  /* expressions/boolean.h */
  rt->registerFunction("eq",  PureFunction(&expressions::eqExpr));
  rt->registerFunction("neq", PureFunction(&expressions::neqExpr));
  rt->registerFunction("and", PureFunction(&expressions::andExpr));
  rt->registerFunction("or",  PureFunction(&expressions::orExpr));
  rt->registerFunction("neg", PureFunction(&expressions::negExpr));
  rt->registerFunction("lt",  PureFunction(&expressions::ltExpr));
  rt->registerFunction("lte", PureFunction(&expressions::lteExpr));
  rt->registerFunction("gt",  PureFunction(&expressions::gtExpr));
  rt->registerFunction("gte", PureFunction(&expressions::gteExpr));

  /* expressions/UnixTime.h */
  rt->registerFunction(
      "FROM_TIMESTAMP",
      PureFunction(&expressions::fromTimestamp));

  /* expressions/math.h */
  rt->registerFunction("add", PureFunction(&expressions::addExpr));
  rt->registerFunction("sub", PureFunction(&expressions::subExpr));
  rt->registerFunction("mul", PureFunction(&expressions::mulExpr));
  rt->registerFunction("div", PureFunction(&expressions::divExpr));
  rt->registerFunction("mod", PureFunction(&expressions::modExpr));
  rt->registerFunction("pow", PureFunction(&expressions::powExpr));

  rt->registerFunction("round", PureFunction(&expressions::roundExpr));
  rt->registerFunction("truncate", PureFunction(&expressions::truncateExpr));

  /* expressions/string.h */
  rt->registerFunction("startswith", PureFunction(&expressions::startsWithExpr));
  rt->registerFunction("endswith", PureFunction(&expressions::endsWithExpr));

  /* expressions/internal.h */
  rt->registerFunction("repeat_value", expressions::kRepeatValueExpr);
}

} // namespace csql
