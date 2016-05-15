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
#include <eventql/util/stdtypes.h>
#include <eventql/util/exception.h>
#include <eventql/util/wallclock.h>
#include <eventql/util/test/unittest.h>
#include "eventql/sql/runtime/defaultruntime.h"
#include "eventql/sql/qtree/SequentialScanNode.h"
#include "eventql/sql/qtree/ColumnReferenceNode.h"
#include "eventql/sql/qtree/CallExpressionNode.h"
#include "eventql/sql/qtree/LiteralExpressionNode.h"
#include "eventql/sql/CSTableScanProvider.h"
#include "eventql/sql/backends/csv/CSVTableProvider.h"

#include "eventql/eventql.h"
using namespace csql;

int main() {
  auto runtime = Runtime::getDefaultRuntime();
  auto ctx = runtime->newTransaction();

  auto estrat = mkRef(new DefaultExecutionStrategy());
  estrat->addTableProvider(new CSTableScanProvider("testtable", "benchmark.cst"));

  ResultList result;
  auto query = R"(
    select
        time / 1000 as time,
        gmv_eurcent,
        num_purchases,
        num_refunds,
        refunded_gmv_eurcent,
        gmv_per_transaction_eurcent,
        refund_rate
    from testtable
    WHERE time >= 1450134000699000 AND time <= 1452726000699000 AND shop_id = 13008
    order by time asc;
  )";

  auto qplan = runtime->buildQueryPlan(ctx.get(), query, estrat.get());
  runtime->executeStatement(ctx.get(), qplan->getStatement(0), &result);
  result.debugPrint();

  return 0;
}
