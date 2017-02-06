/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *   - Laura Schlimmer <laura@eventql.io>
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
#include <eventql/sql/expressions/aggregate.h>
#include <eventql/sql/expressions/boolean.h>
#include <eventql/sql/expressions/datetime.h>
#include <eventql/sql/expressions/math.h>

#include "eventql/eventql.h"

namespace csql {

void installDefaultSymbols(SymbolTable* rt) {
  /* implicit conversions */
  rt->registerImplicitConversion(SType::UINT64, SType::NIL);

  ///* expressions/aggregate.h */
  rt->registerFunction("count", expressions::count);
  rt->registerFunction("sum", expressions::sum_int64);
  rt->registerFunction("sum", expressions::sum_uint64);
  //rt->registerFunction("max", expressions::kMaxExpr);
  //rt->registerFunction("min", expressions::kMinExpr);

  ///* expressions/boolean.h */
  rt->registerFunction("eq",  expressions::eq_uint64);
  rt->registerFunction("eq",  expressions::eq_timestamp64);

  //rt->registerFunction("neq", PureFunction(&expressions::neqExpr));
  rt->registerFunction("logical_and", expressions::logical_and);
  rt->registerFunction("logical_or", expressions::logical_or);
  //rt->registerFunction("neg", PureFunction(&expressions::negExpr));
  rt->registerFunction("lt", expressions::lt_uint64);
  rt->registerFunction("lt", expressions::lt_timestamp64);
  rt->registerFunction("lte", expressions::lte_uint64);
  rt->registerFunction("lte", expressions::lte_timestamp64);
  rt->registerFunction("gt", expressions::gt_uint64);
  rt->registerFunction("gt", expressions::gt_timestamp64);
  rt->registerFunction("gte", expressions::gte_uint64);
  rt->registerFunction("gte", expressions::gte_timestamp64);
  //rt->registerFunction("isnull", PureFunction(&expressions::isNullExpr));

  ///* expressions/conversion.h */
  //rt->registerFunction("to_string", PureFunction(&expressions::toStringExpr));
  //rt->registerFunction("to_str", PureFunction(&expressions::toStringExpr));
  //rt->registerFunction("to_integer", PureFunction(&expressions::toIntExpr));
  //rt->registerFunction("to_int", PureFunction(&expressions::toIntExpr));
  //rt->registerFunction("to_float", PureFunction(&expressions::toFloatExpr));
  //rt->registerFunction("to_bool", PureFunction(&expressions::toBoolExpr));

  ///* expressions/datetime.h */
  //rt->registerFunction("now", PureFunction(&expressions::nowExpr));
  //rt->registerFunction(
  //    "FROM_TIMESTAMP",
  //    PureFunction(&expressions::fromTimestamp));
  //rt->registerFunction("date_trunc", PureFunction(&expressions::dateTruncExpr));
  //rt->registerFunction("date_add", PureFunction(&expressions::dateAddExpr));
  //rt->registerFunction("date_sub", PureFunction(&expressions::dateAddExpr));
  rt->registerFunction("time_at", expressions::time_at);

  ///* expressions/math.h */
  rt->registerFunction("add", expressions::add_uint64);
  rt->registerFunction("add", expressions::add_int64);
  rt->registerFunction("add", expressions::add_float64);
  rt->registerFunction("sub", expressions::sub_uint64);
  rt->registerFunction("sub", expressions::sub_int64);
  rt->registerFunction("sub", expressions::sub_float64);
  rt->registerFunction("mul", expressions::mul_uint64);
  rt->registerFunction("mul", expressions::mul_int64);
  rt->registerFunction("mul", expressions::mul_float64);
  rt->registerFunction("div", expressions::div_uint64);
  rt->registerFunction("div", expressions::div_int64);
  rt->registerFunction("div", expressions::div_float64);
  rt->registerFunction("mod", expressions::mod_uint64);
  rt->registerFunction("mod", expressions::mod_int64);
  rt->registerFunction("mod", expressions::mod_float64);
  rt->registerFunction("pow", expressions::pow_uint64);
  rt->registerFunction("pow", expressions::pow_int64);
  rt->registerFunction("pow", expressions::pow_float64);

  ///* expressions/string.h */
  //rt->registerFunction("startswith", PureFunction(&expressions::startsWithExpr));
  //rt->registerFunction("endswith", PureFunction(&expressions::endsWithExpr));
  //rt->registerFunction("uppercase", PureFunction(&expressions::upperCaseExpr));
  //rt->registerFunction("ucase", PureFunction(&expressions::upperCaseExpr));
  //rt->registerFunction("lowercase", PureFunction(&expressions::lowerCaseExpr));
  //rt->registerFunction("lcase", PureFunction(&expressions::lowerCaseExpr));
  //rt->registerFunction("substring", PureFunction(&expressions::subStringExpr));
  //rt->registerFunction("substr", PureFunction(&expressions::subStringExpr));
  //rt->registerFunction("ltrim", PureFunction(&expressions::ltrimExpr));
  //rt->registerFunction("rtrim", PureFunction(&expressions::rtrimExpr));
  //rt->registerFunction("concat", PureFunction(&expressions::concatExpr));

  ///* expressions/miscellaneous.h */
  //rt->registerFunction("usleep", PureFunction(&expressions::usleepExpr, true));
  //rt->registerFunction("fnv32", PureFunction(&expressions::fnv32Expr));

  ///* expressions/internal.h */
  //rt->registerFunction("repeat_value", expressions::kRepeatValueExpr);

}

} // namespace csql
