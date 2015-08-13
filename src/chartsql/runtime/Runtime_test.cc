/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
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
#include "chartsql/runtime/defaultruntime.h"
#include "chartsql/qtree/SequentialScanNode.h"
#include "chartsql/qtree/ColumnReferenceNode.h"
#include "chartsql/qtree/CallExpressionNode.h"
#include "chartsql/qtree/LiteralExpressionNode.h"

using namespace stx;
using namespace csql;

UNIT_TEST(RuntimeTest);

TEST_CASE(RuntimeTest, TestStaticExpression, [] () {
  auto runtime = Runtime::getDefaultRuntime();

  auto expr = mkRef(
      new csql::CallExpressionNode(
          "add",
          {
            new csql::LiteralExpressionNode(SValue(SValue::IntegerType(1))),
            new csql::LiteralExpressionNode(SValue(SValue::IntegerType(2))),
          }));

  auto t0 = WallClock::unixMicros();
  SValue out;
  for (int i = 0; i < 1000; ++i) {
    out = runtime->evaluateStaticExpression(expr.get());
  }
  auto t1 = WallClock::unixMicros();

  EXPECT_EQ(out.getInteger(), 3);
});

TEST_CASE(RuntimeTest, TestExecuteIfStatement, [] () {
  auto runtime = Runtime::getDefaultRuntime();

  auto out_a = runtime->evaluateStaticExpression("if(1 = 1, 42, 23)");
  EXPECT_EQ(out_a.getInteger(), 42);

  auto out_b = runtime->evaluateStaticExpression("if(1 = 2, 42, 23)");
  EXPECT_EQ(out_b.getInteger(), 23);

  {
    auto v = runtime->evaluateStaticExpression("if(1 = 1, 'fnord', 'blah')");
    EXPECT_EQ(v.toString(), "fnord");
  }

  {
    auto v = runtime->evaluateStaticExpression("if(1 = 2, 'fnord', 'blah')");
    EXPECT_EQ(v.toString(), "blah");
  }

  {
    auto v = runtime->evaluateStaticExpression("if('fnord' = 'blah', 1, 2)");
    EXPECT_EQ(v.toString(), "2");
  }

  {
    auto v = runtime->evaluateStaticExpression("if('fnord' = 'fnord', 1, 2)");
    EXPECT_EQ(v.toString(), "1");
  }

  {
    auto v = runtime->evaluateStaticExpression("if('fnord' = '', 1, 2)");
    EXPECT_EQ(v.toString(), "2");
  }
});
