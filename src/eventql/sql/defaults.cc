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
#include <eventql/sql/expressions/conversion.h>
#include <eventql/sql/expressions/datetime.h>
#include <eventql/sql/expressions/math.h>
#include <eventql/sql/expressions/string.h>

#include "eventql/eventql.h"

namespace csql {

void installDefaultSymbols(SymbolTable* rt) {
  /* implicit conversions */
  rt->registerImplicitConversion(SType::UINT64, SType::NIL);
  rt->registerImplicitConversion(SType::UINT64, SType::INT64);
  rt->registerImplicitConversion(SType::INT64, SType::NIL);
  rt->registerImplicitConversion(SType::FLOAT64, SType::NIL);
  rt->registerImplicitConversion(SType::BOOL, SType::NIL);
  rt->registerImplicitConversion(SType::STRING, SType::NIL);
  rt->registerImplicitConversion(SType::TIMESTAMP64, SType::NIL);

  /* expressions/aggregate.h */
  rt->registerFunction("count", expressions::count);
  rt->registerFunction("sum", expressions::sum_int64);
  rt->registerFunction("sum", expressions::sum_uint64);
  //rt->registerFunction("max", expressions::kMaxExpr);
  //rt->registerFunction("min", expressions::kMinExpr);

  /* expressions/boolean.h */
  rt->registerFunction("logical_and", expressions::logical_and);
  rt->registerFunction("logical_or", expressions::logical_or);
  rt->registerFunction("neg", expressions::neg);
  rt->registerFunction("cmp",  expressions::cmp_uint64);
  rt->registerFunction("cmp",  expressions::cmp_int64);
  rt->registerFunction("cmp",  expressions::cmp_float64);
  rt->registerFunction("cmp",  expressions::cmp_timestamp64);
  rt->registerFunction("eq",  expressions::eq_uint64);
  rt->registerFunction("eq",  expressions::eq_int64);
  rt->registerFunction("eq",  expressions::eq_float64);
  rt->registerFunction("eq",  expressions::eq_bool);
  rt->registerFunction("eq",  expressions::eq_string);
  rt->registerFunction("eq",  expressions::eq_timestamp64);
  rt->registerFunction("neq",  expressions::neq_uint64);
  rt->registerFunction("neq",  expressions::neq_int64);
  rt->registerFunction("neq",  expressions::neq_float64);
  rt->registerFunction("neq",  expressions::neq_bool);
  rt->registerFunction("neq",  expressions::neq_string);
  rt->registerFunction("neq",  expressions::neq_timestamp64);
  rt->registerFunction("lt", expressions::lt_uint64);
  rt->registerFunction("lt", expressions::lt_int64);
  rt->registerFunction("lt", expressions::lt_float64);
  rt->registerFunction("lt", expressions::lt_timestamp64);
  rt->registerFunction("lte", expressions::lte_uint64);
  rt->registerFunction("lte", expressions::lte_float64);
  rt->registerFunction("lte", expressions::lte_timestamp64);
  rt->registerFunction("lte", expressions::lte_timestamp64);
  rt->registerFunction("gt", expressions::gt_uint64);
  rt->registerFunction("gt", expressions::gt_int64);
  rt->registerFunction("gt", expressions::gt_float64);
  rt->registerFunction("gt", expressions::gt_timestamp64);
  rt->registerFunction("gte", expressions::gte_uint64);
  rt->registerFunction("gte", expressions::gte_int64);
  rt->registerFunction("gte", expressions::gte_float64);
  rt->registerFunction("gte", expressions::gte_timestamp64);
  //rt->registerFunction("isnull", PureFunction(&expressions::isNullExpr));

  /* expressions/conversion.h */
  rt->registerFunction("to_nil", expressions::to_nil_uint64);
  rt->registerFunction("to_nil", expressions::to_nil_int64);
  rt->registerFunction("to_nil", expressions::to_nil_float64);
  rt->registerFunction("to_nil", expressions::to_nil_bool);
  rt->registerFunction("to_nil", expressions::to_nil_string);
  rt->registerFunction("to_nil", expressions::to_nil_timestamp64);
  rt->registerFunction("to_int64", expressions::to_int64_uint64);
  rt->registerFunction("to_int64", expressions::to_int64_float64);
  rt->registerFunction("to_int64", expressions::to_int64_bool);
  rt->registerFunction("to_int64", expressions::to_int64_timestamp64);
  rt->registerFunction("to_string", expressions::to_string_nil);
  rt->registerFunction("to_string", expressions::to_string_uint64);
  rt->registerFunction("to_string", expressions::to_string_int64);
  rt->registerFunction("to_string", expressions::to_string_float64);
  rt->registerFunction("to_string", expressions::to_string_bool);
  rt->registerFunction("to_string", expressions::to_string_timestamp64);
  rt->registerFunction("to_timestamp64", expressions::to_timestamp64_int64);
  rt->registerFunction("to_timestamp64", expressions::to_timestamp64_float64);

  /* expressions/datetime.h */
  rt->registerFunction("now", expressions::now);
  rt->registerFunction("from_timestamp", expressions::from_timestamp_int64);
  rt->registerFunction("from_timestamp", expressions::from_timestamp_float64);
  rt->registerFunction("date_trunc", expressions::date_trunc_timestamp64);
  //rt->registerFunction("date_add", PureFunction(&expressions::dateAddExpr));
  //rt->registerFunction("date_sub", PureFunction(&expressions::dateAddExpr));
  rt->registerFunction("time_at", expressions::time_at);

  /* expressions/math.h */
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

  /* expressions/string.h */
  //rt->registerFunction("startswith", PureFunction(&expressions::startsWithExpr));
  //rt->registerFunction("endswith", PureFunction(&expressions::endsWithExpr));
  rt->registerFunction("lcase", expressions::lcase);
  rt->registerFunction("lowercase", expressions::lcase);
  rt->registerFunction("ucase", expressions::ucase);
  rt->registerFunction("uppercase", expressions::ucase);
  //rt->registerFunction("substring", PureFunction(&expressions::subStringExpr));
  //rt->registerFunction("substr", PureFunction(&expressions::subStringExpr));
  //rt->registerFunction("ltrim", PureFunction(&expressions::ltrimExpr));
  //rt->registerFunction("rtrim", PureFunction(&expressions::rtrimExpr));
  //rt->registerFunction("concat", PureFunction(&expressions::concatExpr));

  /* expressions/miscellaneous.h */
  //rt->registerFunction("usleep", PureFunction(&expressions::usleepExpr, true));
  //rt->registerFunction("fnv32", PureFunction(&expressions::fnv32Expr));

  /* expressions/internal.h */
  //rt->registerFunction("repeat_value", expressions::kRepeatValueExpr);

}

} // namespace csql
