/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <fnord/stdtypes.h>
#include <fnord/exception.h>
#include <fnord/wallclock.h>
#include <fnord/test/unittest.h>
#include "chartsql/runtime/defaultruntime.h"
#include "chartsql/qtree/SequentialScanNode.h"
#include "chartsql/qtree/ColumnReferenceNode.h"
#include "chartsql/qtree/CallExpressionNode.h"
#include "chartsql/qtree/LiteralExpressionNode.h"

using namespace fnord;
using namespace csql;

UNIT_TEST(RuntimeTest);

TEST_CASE(RuntimeTest, TestStaticExpression, [] () {
  Runtime runtime;

  auto expr = mkRef(
      new csql::CallExpressionNode(
          "add",
          {
            new csql::LiteralExpressionNode(SValue(SValue::IntegerType(1))),
            new csql::LiteralExpressionNode(SValue(SValue::IntegerType(2))),
          }));

  auto t0 = WallClock::unixMicros();
  SValue out;
  for (int i = 0; i < 1000000; ++i) {
    out = runtime.evaluateStaticExpression(expr.get());
  }
  auto t1 = WallClock::unixMicros();

  EXPECT_EQ(out.getInteger(), 3);
});

