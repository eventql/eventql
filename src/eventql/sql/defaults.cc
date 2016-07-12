/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *   - Laura Schlimmer <laura@zscale.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#include <eventql/sql/defaults.h>

#include "eventql/eventql.h"

namespace csql {

void installDefaultSymbols(SymbolTable* rt) {
  /* expressions/aggregate.h */
  rt->registerFunction("count", expressions::kCountExpr);
  rt->registerFunction("sum", expressions::kSumExpr);
  rt->registerFunction("max", expressions::kMaxExpr);
  rt->registerFunction("min", expressions::kMinExpr);

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
  rt->registerFunction("logical_and", PureFunction(&expressions::andExpr));
  rt->registerFunction("logical_or", PureFunction(&expressions::orExpr));
  rt->registerFunction("neg", PureFunction(&expressions::negExpr));
  rt->registerFunction("lt",  PureFunction(&expressions::ltExpr));
  rt->registerFunction("lte", PureFunction(&expressions::lteExpr));
  rt->registerFunction("gt",  PureFunction(&expressions::gtExpr));
  rt->registerFunction("gte", PureFunction(&expressions::gteExpr));
  rt->registerFunction("isnull", PureFunction(&expressions::isNullExpr));

  /* expressions/conversion.h */
  rt->registerFunction("to_string", PureFunction(&expressions::toStringExpr));
  rt->registerFunction("to_str", PureFunction(&expressions::toStringExpr));
  rt->registerFunction("to_integer", PureFunction(&expressions::toIntExpr));
  rt->registerFunction("to_int", PureFunction(&expressions::toIntExpr));
  rt->registerFunction("to_float", PureFunction(&expressions::toFloatExpr));
  rt->registerFunction("to_bool", PureFunction(&expressions::toBoolExpr));

  /* expressions/datetime.h */
  rt->registerFunction("now", PureFunction(&expressions::nowExpr));
  rt->registerFunction(
      "FROM_TIMESTAMP",
      PureFunction(&expressions::fromTimestamp));
  rt->registerFunction("date_trunc", PureFunction(&expressions::dateTruncExpr));
  rt->registerFunction("date_add", PureFunction(&expressions::dateAddExpr));
  rt->registerFunction("date_sub", PureFunction(&expressions::dateAddExpr));
  rt->registerFunction("time_at", PureFunction(&expressions::timeAtExpr));

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
  rt->registerFunction("uppercase", PureFunction(&expressions::upperCaseExpr));
  rt->registerFunction("ucase", PureFunction(&expressions::upperCaseExpr));
  rt->registerFunction("lowercase", PureFunction(&expressions::lowerCaseExpr));
  rt->registerFunction("lcase", PureFunction(&expressions::lowerCaseExpr));
  rt->registerFunction("substring", PureFunction(&expressions::subStringExpr));
  rt->registerFunction("substr", PureFunction(&expressions::subStringExpr));
  rt->registerFunction("ltrim", PureFunction(&expressions::ltrimExpr));
  rt->registerFunction("rtrim", PureFunction(&expressions::rtrimExpr));

  /* expressions/internal.h */
  rt->registerFunction("repeat_value", expressions::kRepeatValueExpr);
}

} // namespace csql
