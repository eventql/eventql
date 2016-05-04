/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *   Copyright (c) 2015 Laura Schlimmer
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/stdtypes.h>
#include <stx/exception.h>
#include <stx/wallclock.h>
#include <stx/test/unittest.h>
#include "csql/runtime/defaultruntime.h"
#include "csql/qtree/SequentialScanNode.h"
#include "csql/qtree/ColumnReferenceNode.h"
#include "csql/qtree/CallExpressionNode.h"
#include "csql/qtree/LiteralExpressionNode.h"
#include "csql/CSTableScanProvider.h"
#include "csql/backends/csv/CSVTableProvider.h"

using namespace stx;
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
