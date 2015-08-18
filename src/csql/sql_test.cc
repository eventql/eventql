/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <csql/backends/csv/csvbackend.h>
#include <csql/backends/csv/csvtableref.h>
#include <csql/backends/tableref.h>
#include <csql/parser/parser.h>
#include <csql/parser/token.h>
#include <csql/parser/tokenize.h>
#include <csql/runtime/defaultruntime.h>
#include <csql/runtime/queryplannode.h>
#include <csql/runtime/resultlist.h>
#include <csql/runtime/tablescan.h>
#include <csql/runtime/tablerepository.h>
#include <ui/canvas.h>
#include <ui/svgtarget.h>
#include <util/UnixTime.h>
#include <util/inputstream.h>
#include <util/outputstream.h>
#include <util/unittest.h>
#include <stx/exception.h>

using namespace stxmetric::query;

UNIT_TEST(SQLTest);

class TestTableRef : public TableRef {
  std::vector<std::string> columns() override {
    return {"one", "two", "three"};
  }
  int getColumnIndex(const std::string& name) override {
    if (name == "one") return 0;
    if (name == "two") return 1;
    if (name == "three") return 2;
    return -1;
  }
  std::string getColumnName(int index) override {
    return columns()[index];
  }
  void executeScan(TableScan* scan) override {
    int64_t one = 0;
    int64_t two = 100;
    for (int i = two; i > 0; --i) {
      std::vector<SValue> row;
      row.emplace_back(SValue(++one));
      row.emplace_back(SValue(two--));
      row.emplace_back(SValue((int64_t) (i % 2 ? 10 : 20)));
      if (!scan->nextRow(row.data(), row.size())) {
        return;
      }
    }
  }
};

class TestTable2Ref : public TableRef {
  std::vector<std::string> columns() override {
    return {"one", "two", "three"};
  }
  int getColumnIndex(const std::string& name) override {
    if (name == "one") return 0;
    if (name == "two") return 1;
    if (name == "three") return 2;
    return -1;
  }
  std::string getColumnName(int index) override {
    return columns()[index];
  }
  void executeScan(TableScan* scan) override {
    for (int i = 10; i > 0; --i) {
      std::vector<SValue> row;
      row.emplace_back(SValue((int64_t) i));
      row.emplace_back(SValue((int64_t) (i * 2)));
      row.emplace_back(SValue((int64_t) (i % 2 ? 100 : 200)));
      if (!scan->nextRow(row.data(), row.size())) {
        return;
      }
    }
  }
};

class TestTimeTableRef : public TableRef {
  std::vector<std::string> columns() override {
    return {"time", "value"};
  }
  int getColumnIndex(const std::string& name) override {
    if (name == "time") return 0;
    if (name == "value") return 1;
    return -1;
  }
  std::string getColumnName(int index) override {
    return columns()[index];
  }
  void executeScan(TableScan* scan) override {
    auto start_time = 1415712875216794;

    for (int i = 0; i < 500; ++i) {
      std::vector<SValue> row;
      if (i == 300) {
        start_time += 120000000;
      }

      row.emplace_back(stx::util::UnixTime(start_time + 1000000 * i));
      row.emplace_back(SValue((SValue::IntegerType) i));
      if (!scan->nextRow(row.data(), row.size())) {
        return;
      }
    }
  }
};

static std::unique_ptr<ResultList> executeTestQuery(
    const char* query) {
  DefaultRuntime runtime;
  runtime.addBackend(
      std::unique_ptr<Backend>(new csv_backend::CSVBackend()));

  TableRepository table_repo;
  QueryPlan query_plan(&table_repo);
  query_plan.tableRepository()->addTableRef(
      "testtable",
      std::unique_ptr<TableRef>(new TestTableRef()));

  query_plan.tableRepository()->addTableRef(
      "testtable2",
      std::unique_ptr<TableRef>(new TestTable2Ref()));

  query_plan.tableRepository()->addTableRef(
      "timeseries",
      std::unique_ptr<TableRef>(new TestTimeTableRef()));

  query_plan.tableRepository()->addTableRef(
      "gbp_per_country",
      std::unique_ptr<TableRef>(
          new csv_backend::CSVTableRef(
              csv_backend::CSVInputStream::openFile(
                  "test/fixtures/gbp_per_country_simple.csv"), true)));

  query_plan.tableRepository()->addTableRef(
      "gdp_per_capita",
      std::unique_ptr<TableRef>(
          new csv_backend::CSVTableRef(
              csv_backend::CSVInputStream::openFile(
                  "test/fixtures/gdp_per_capita.csv"), true)));

  auto ast = runtime.parser()->parseQuery(query);
  runtime.queryPlanBuilder()->buildQueryPlan(ast, &query_plan);
  EXPECT(query_plan.queries().size() == 1);

  auto result = new ResultList();
  auto query_plan_node = query_plan.queries()[0].get();
  result->addHeader(query_plan_node->getColumns());
  query_plan_node->setTarget(result);
  query_plan_node->execute();

  EXPECT(result->getNumRows() > 0);
  EXPECT(result->getNumColumns() > 0);
  return std::unique_ptr<ResultList>(result);
}



TEST_CASE(SQLTest, TestSelectOnlyQuery, [] () {
  auto results = executeTestQuery(
      "  SELECT"
      "    13 + 2 * 5 as fnord,"
      "    2 ^ 2 ^ 3 as fubar,"
      "    13 * (8 % 3) + -5 as baz,"
      "    true one,"
      "    !(true) as two,"
      "    NOT NOT true as three;");

  EXPECT(results->getNumColumns() == 6);
  EXPECT(results->getNumRows() == 1);
  const auto& cols = results->getColumns();
  EXPECT(cols[0] == "fnord");
  EXPECT(cols[1] == "fubar");
  EXPECT(cols[2] == "baz");
  EXPECT(cols[3] == "one");
  EXPECT(cols[4] == "two");
  EXPECT(cols[5] == "three");
  const auto& row = results->getRow(0);
  EXPECT(row[0] == "23");
  EXPECT(row[1] == "256");
  EXPECT(row[2] == "21");
  //EXPECT(row[3] == "true");
  //EXPECT(row[4] == "false");
  //EXPECT(row[5] == "true");
});

TEST_CASE(SQLTest, TestSimpleTableScanQuery, [] () {
  auto results = executeTestQuery(
      "  SELECT one + 50, two FROM testtable");

  EXPECT(results->getNumColumns() == 2);
  EXPECT(results->getNumRows() == 100);

  for (int i = 0; i<100; ++i) {
    const auto& row = results->getRow(i);
    EXPECT(atoi(row[0].c_str()) == 51 + i);
    EXPECT(atoi(row[1].c_str()) == 100 - i);
  }
});

TEST_CASE(SQLTest, TestTableScanWhereQuery, [] () {
  auto results = executeTestQuery(
      "  SELECT"
      "    one + 1 as fnord,"
      "    two"
      "  FROM"
      "    testtable"
      "  WHERE"
      "    one > two or one = 3;");

  EXPECT(results->getNumRows() == 51);
});

TEST_CASE(SQLTest, TestSelectAllQuery, [] () {
  auto results = executeTestQuery("SELECT * from testtable;");
  EXPECT(results->getNumColumns() == 3);
  EXPECT(results->getNumRows() == 100);
});

TEST_CASE(SQLTest, TestSelectTableAllQuery, [] () {
  auto results = executeTestQuery("SELECT testtable.* from testtable;");
  EXPECT(results->getNumColumns() == 3);
  EXPECT(results->getNumRows() == 100);
});

TEST_CASE(SQLTest, TestSelectAllMultitable, [] () {
  auto expected = "can't use wilcard select (SELECT * FROM ...) when selecting "
      "from multiple tables";

  EXPECT_EXCEPTION(expected, [] () {
    auto results = executeTestQuery("SELECT * from testtable, tesstable2;");
  });

  EXPECT_EXCEPTION(expected, [] () {
    auto results = executeTestQuery("SELECT testtable.*, tesstable2.* "
        "from testtable, tesstable2;");
  });
});

TEST_CASE(SQLTest, TestTableScanWhereLimitQuery, [] () {
  auto results = executeTestQuery(
      "  SELECT"
      "    one + 1 as fnord,"
      "    two"
      "  FROM"
      "    testtable"
      "  WHERE"
      "    one > two or one = 3"
      "  LIMIT 10 OFFSET 5;");

  EXPECT(results->getNumRows() == 10);
  const auto& row = results->getRow(0);
  EXPECT(row[0] == "56");
  EXPECT(row[1] == "46");
});

// select count(*), one, two, three from testtable2 group by case three when
// 200 then 100 else 100 end;

TEST_CASE(SQLTest, TestTableScanGroupByQuery, [] () {
  auto results = executeTestQuery(
      "  SELECT"
      "    one,"
      "    two,"
      "    three"
      "  FROM"
      "    testtable2"
      "  GROUP BY"
      "    three, "
      "    two % 8;");

  EXPECT(results->getNumRows() == 4);
});

TEST_CASE(SQLTest, TestTableScanGroupByCountQuery, [] () {
  auto results = executeTestQuery(
      "  SELECT"
      "    count(one),"
      "    one,"
      "    two,"
      "    three"
      "  FROM"
      "    testtable2"
      "  GROUP BY"
      "    three, "
      "    two % 8;");

  int sum = 0;
  for (int i = 0; i < results->getNumRows(); ++i) {
    const auto& row = results->getRow(i);
    sum += atoi(row[0].c_str());
  }
  EXPECT(sum == 10);
});

TEST_CASE(SQLTest, TestCountAllQuery, [] () {
  auto results = executeTestQuery(
      "  SELECT count(*) FROM testtable2;");

  EXPECT(results->getNumRows() == 1);
  EXPECT_EQ(results->getRow(0)[0], "10");
});


TEST_CASE(SQLTest, TestTableScanGroupBySumQuery, [] () {
  auto results = executeTestQuery(
      "  SELECT"
      "    sum(one),"
      "    three"
      "  FROM"
      "    testtable2"
      "  GROUP BY"
      "    three;");

  EXPECT(results->getNumRows() == 2);
  for (int i = 0; i<2; ++i) {
    const auto& row = results->getRow(i);
    EXPECT(
      (atoi(row[0].c_str()) == 25 && atoi(row[1].c_str()) == 100) ||
      (atoi(row[0].c_str()) == 30 && atoi(row[1].c_str()) == 200));
  }
});

TEST_CASE(SQLTest, TestTableScanGroupWithoutGroupClause, [] () {
  auto results = executeTestQuery(
      "  SELECT"
      "    sum(one)"
      "  FROM"
      "    testtable2;");

  EXPECT(results->getNumRows() == 1);
  EXPECT(results->getRow(0)[0] == "55");
});

TEST_CASE(SQLTest, TestSimpleSelectFromCSV, [] () {
  auto results = executeTestQuery(
      "  SELECT"
      "    country as country, gbp as gbp"
      "  FROM"
      "    gbp_per_country;");

  EXPECT_EQ(results->getNumRows(), 191);
});

TEST_CASE(SQLTest, TestSimpleAggregateFromCSV, [] () {
  auto results = executeTestQuery(
      "  SELECT"
      "    sum(gbp) as global_gbp"
      "  FROM"
      "    gbp_per_country;");

  EXPECT_EQ(std::stof(results->getRow(0)[0]), 74209240);
});

TEST_CASE(SQLTest, TestCaseInsensitiveFunctionNames, [] () {
  auto results = executeTestQuery(
      "  SELECT"
      "    sUM(gbp) as global_gbp"
      "  FROM"
      "    gbp_per_country;");

  EXPECT_EQ(std::stof(results->getRow(0)[0]), 74209240);
});

TEST_CASE(SQLTest, TestSimpleOrderByAsc, [] () {
  auto results = executeTestQuery(
      "  SELECT"
      "    country, gbp"
      "  FROM"
      "    gbp_per_country"
      "  ORDER BY"
      "    country ASC;");

  EXPECT_EQ(results->getNumColumns(), 2);
  EXPECT_EQ(results->getNumRows(), 191);
  EXPECT_EQ(results->getRow(0)[0], "ABW");
  EXPECT_EQ(results->getRow(0)[1], "2584");
  EXPECT_EQ(results->getRow(190)[0], "ZWE");
  EXPECT_EQ(results->getRow(190)[1], "12802");
});

TEST_CASE(SQLTest, TestSimpleOrderByDesc, [] () {
  auto results = executeTestQuery(
      "  SELECT"
      "    country, gbp"
      "  FROM"
      "    gbp_per_country"
      "  ORDER BY"
      "    country DESC;");

  EXPECT_EQ(results->getNumColumns(), 2);
  EXPECT_EQ(results->getNumRows(), 191);
  EXPECT_EQ(results->getRow(0)[0], "ZWE");
  EXPECT_EQ(results->getRow(0)[1], "12802");
  EXPECT_EQ(results->getRow(190)[0], "ABW");
  EXPECT_EQ(results->getRow(190)[1], "2584");
});

TEST_CASE(SQLTest, TestSimpleOrderByWithoutOrderingSpec, [] () {
  auto results = executeTestQuery(
      "  SELECT"
      "    country, gbp"
      "  FROM"
      "    gbp_per_country"
      "  ORDER BY"
      "    country;");

  EXPECT_EQ(results->getNumColumns(), 2);
  EXPECT_EQ(results->getNumRows(), 191);
  EXPECT_EQ(results->getRow(0)[0], "ABW");
  EXPECT_EQ(results->getRow(0)[1], "2584");
  EXPECT_EQ(results->getRow(190)[0], "ZWE");
  EXPECT_EQ(results->getRow(190)[1], "12802");
});

/*
TEST_CASE(SQLTest, TestSimpleOrderByAscWithWildcardSelect, [] () {
  auto results = executeTestQuery(
      "  SELECT"
      "    *"
      "  FROM"
      "    gbp_per_country"
      "  ORDER BY"
      "    country ASC;");

  results->debugPrint();
  EXPECT_EQ(results->getRow(0)[1], "ABW");
});
*/

TEST_CASE(SQLTest, TestSimpleOrderByAscOnUnselectedColumn, [] () {
  auto results = executeTestQuery(
      "  SELECT"
      "    country"
      "  FROM"
      "    gbp_per_country"
      "  ORDER BY"
      "    gbp ASC;");

  EXPECT_EQ(results->getNumColumns(), 1);
  EXPECT_EQ(results->getNumRows(), 191);
  EXPECT_EQ(results->getRow(0)[0], "TUV");
  EXPECT_EQ(results->getRow(190)[0], "USA");
});


TEST_CASE(SQLTest, TestNoSuchColumnError, [] () {
  EXPECT_EXCEPTION("no such column: 'fnord'", [] () {
    auto results = executeTestQuery(
        "  SELECT"
        "    sum(fnord) as global_gbp"
        "  FROM"
        "    gbp_per_country;");
  });
});

TEST_CASE(SQLTest, TestTypeError, [] () {
  EXPECT_EXCEPTION("can't convert String 'USA' to Float", [] () {
    auto results = executeTestQuery(
        "  SELECT"
        "    sum(country) as global_gbp"
        "  FROM"
        "    gbp_per_country;");
  });
});

TEST_CASE(SQLTest, TestImportCSVTable, [] () {
  auto results = executeTestQuery(
      "  IMPORT TABLE gbp_per_country "
      "     FROM 'csv:test/fixtures/gbp_per_country_simple.csv?headers=true';"
      ""
      "  SELECT"
      "    sum(gbp) as global_gbp"
      "  FROM"
      "    gbp_per_country;");

  EXPECT(std::stof(results->getRow(0)[0]) == 74209240);
});

TEST_CASE(SQLTest, TestEquals, [] () {
  auto result = executeTestQuery(
      "  IMPORT TABLE city_temperatures "
      "     FROM 'csv:test/fixtures/city_temperatures.csv?headers=true';"
      ""
      "  SELECT city FROM city_temperatures WHERE city = 'Berlin'"
      "     GROUP BY city LIMIT 10;");

  EXPECT(result->getNumRows() == 1);
  EXPECT(result->getRow(0)[0] == "Berlin");
});

TEST_CASE(SQLTest, TestNotEquals, [] () {
  auto result = executeTestQuery(
      "  IMPORT TABLE city_temperatures "
      "     FROM 'csv:test/fixtures/city_temperatures.csv?headers=true';"
      ""
      "  SELECT city FROM city_temperatures WHERE city != 'Berlin'"
      "     GROUP BY city LIMIT 10;");

  EXPECT(result->getNumRows() == 3);
  EXPECT(result->getRow(0)[0] == "London");
  EXPECT(result->getRow(1)[0] == "New York");
  EXPECT(result->getRow(2)[0] == "Tokyo");
});

TEST_CASE(SQLTest, TestLessThan, [] () {
  auto result = executeTestQuery(
      "  IMPORT TABLE city_temperatures "
      "     FROM 'csv:test/fixtures/city_temperatures.csv?headers=true';"
      ""
      "  SELECT city FROM city_temperatures WHERE city < 'New York'"
      "     GROUP BY city LIMIT 10;");

  EXPECT(result->getNumRows() == 2);
  EXPECT(result->getRow(0)[0] == "London");
  EXPECT(result->getRow(1)[0] == "Berlin");
});

TEST_CASE(SQLTest, TestLessThanEquals, [] () {
  auto result = executeTestQuery(
      "  IMPORT TABLE city_temperatures "
      "     FROM 'csv:test/fixtures/city_temperatures.csv?headers=true';"
      ""
      "  SELECT city FROM city_temperatures WHERE city <= 'New York'"
      "     GROUP BY city LIMIT 10;");

  EXPECT(result->getNumRows() == 3);
  EXPECT(result->getRow(0)[0] == "London");
  EXPECT(result->getRow(1)[0] == "Berlin");
  EXPECT(result->getRow(2)[0] == "New York");
});

TEST_CASE(SQLTest, TestGreaterThan, [] () {
  auto result = executeTestQuery(
      "  IMPORT TABLE city_temperatures "
      "     FROM 'csv:test/fixtures/city_temperatures.csv?headers=true';"
      ""
      "  SELECT city FROM city_temperatures WHERE city > 'New York'"
      "     GROUP BY city LIMIT 10;");

  EXPECT(result->getNumRows() == 1);
  EXPECT(result->getRow(0)[0] == "Tokyo");
});

TEST_CASE(SQLTest, TestGreaterThanEquals, [] () {
  auto result = executeTestQuery(
      "  IMPORT TABLE city_temperatures "
      "     FROM 'csv:test/fixtures/city_temperatures.csv?headers=true';"
      ""
      "  SELECT city FROM city_temperatures WHERE city >= 'New York'"
      "     GROUP BY city LIMIT 10;");

  EXPECT(result->getNumRows() == 2);
  EXPECT(result->getRow(0)[0] == "New York");
  EXPECT(result->getRow(1)[0] == "Tokyo");
});

TEST_CASE(SQLTest, TestDoubleEqualsSignError, [] () {
  TableRepository repo;
  auto query_str =
      "  IMPORT TABLE city_temperatures "
      "     FROM 'csv:test/fixtures/city_temperatures.csv?headers=true';"
      ""
      "  SELECT city FROM city_temperatures WHERE city == 'Berlin'";

  EXPECT_EXCEPTION(
      "eqExpr needs second argument. Did you type '==' instead of '='?",
      [&] () {
        auto result = executeTestQuery(query_str);
      });
});

TEST_CASE(SQLTest, TestOrderByWithColumnAlias, [] () {
  auto result = executeTestQuery(
      "  SELECT year AS series, isocode AS x, gdp AS y"
      "      FROM gdp_per_capita"
      "      WHERE year = '2010' OR year = '2009' or year = '2008'"
      "      ORDER BY gdp DESC, year DESC"
      "      LIMIT 9;");

  EXPECT_EQ(result->getNumRows(), 9);
  EXPECT_EQ(result->getNumColumns(), 3);
  EXPECT_EQ(result->getColumns()[0], "series");
  EXPECT_EQ(result->getColumns()[1], "x");
  EXPECT_EQ(result->getColumns()[2], "y");
  EXPECT_EQ(result->getRow(0)[0], "2010");
  EXPECT_EQ(result->getRow(0)[1], "QAT");
  EXPECT_EQ(result->getRow(1)[0], "2009");
  EXPECT_EQ(result->getRow(1)[1], "QAT");
  EXPECT_EQ(result->getRow(2)[0], "2008");
  EXPECT_EQ(result->getRow(2)[1], "QAT");
  EXPECT_EQ(result->getRow(3)[0], "2008");
  EXPECT_EQ(result->getRow(3)[1], "LUX");
  EXPECT_EQ(result->getRow(4)[0], "2010");
  EXPECT_EQ(result->getRow(4)[1], "LUX");
  EXPECT_EQ(result->getRow(5)[0], "2009");
  EXPECT_EQ(result->getRow(5)[1], "LUX");
  EXPECT_EQ(result->getRow(6)[0], "2008");
  EXPECT_EQ(result->getRow(6)[1], "ARE");
  EXPECT_EQ(result->getRow(7)[0], "2009");
  EXPECT_EQ(result->getRow(7)[1], "ARE");
  EXPECT_EQ(result->getRow(8)[0], "2010");
  EXPECT_EQ(result->getRow(8)[1], "ARE");
});

TEST_CASE(SQLTest, TestOrderByMulti, [] () {
  auto result = executeTestQuery(
      "  SELECT year, isocode, gdp"
      "      FROM gdp_per_capita"
      "      WHERE year = '2010' OR year = '2009' or year = '2008'"
      "      ORDER BY country DESC, year ASC;");

  EXPECT_EQ(result->getNumRows(), 570);
  EXPECT_EQ(result->getNumColumns(), 3);
  EXPECT_EQ(result->getColumns()[0], "year");
  EXPECT_EQ(result->getColumns()[1], "isocode");
  EXPECT_EQ(result->getColumns()[2], "gdp");
  EXPECT_EQ(result->getRow(0)[0], "2008");
  EXPECT_EQ(result->getRow(0)[1], "ZWE");
  EXPECT_EQ(result->getRow(1)[0], "2009");
  EXPECT_EQ(result->getRow(1)[1], "ZWE");
  EXPECT_EQ(result->getRow(2)[0], "2010");
  EXPECT_EQ(result->getRow(2)[1], "ZWE");
  EXPECT_EQ(result->getRow(567)[0], "2008");
  EXPECT_EQ(result->getRow(567)[1], "AFG");
  EXPECT_EQ(result->getRow(568)[0], "2009");
  EXPECT_EQ(result->getRow(568)[1], "AFG");
  EXPECT_EQ(result->getRow(569)[0], "2010");
  EXPECT_EQ(result->getRow(569)[1], "AFG");
});

/*
TEST_CASE(SQLTest, TestOrderByGroupByTableCol, [] () {
  auto result = executeTestQuery(
      "  SELECT country"
      "      FROM gdp_per_capita"
      "      GROUP BY gdp_per_capita.country"
      "      ORDER BY gdp_per_capita.country DESC;");

  EXPECT_EQ(result->getNumRows(), 570);
  EXPECT_EQ(result->getNumColumns(), 1);
  EXPECT_EQ(result->getColumns()[0], "country");
  EXPECT_EQ(result->getRow(0)[0], "ZWE");
  EXPECT_EQ(result->getRow(569)[0], "AFG");
});
*/

TEST_CASE(SQLTest, TestOrderByTableCol, [] () {
  auto result = executeTestQuery(
      "  SELECT isocode, country"
      "      FROM gdp_per_capita"
      "      ORDER BY gdp_per_capita.country DESC;");

  EXPECT_EQ(result->getNumRows(), 2090);
  EXPECT_EQ(result->getNumColumns(), 2);
  EXPECT_EQ(result->getColumns()[0], "isocode");
  EXPECT_EQ(result->getColumns()[1], "country");
  EXPECT_EQ(result->getRow(0)[0], "ZWE");
  EXPECT_EQ(result->getRow(0)[1], "Zimbabwe");
  EXPECT_EQ(result->getRow(2089)[0], "AFG");
  EXPECT_EQ(result->getRow(2089)[1], "Afghanistan");
});

TEST_CASE(SQLTest, TestRuntime, [] () {
  DefaultRuntime runtime;
  runtime.addBackend(std::unique_ptr<Backend>(new csv_backend::CSVBackend()));

  auto ast = runtime.parser()->parseQuery(
      "  IMPORT TABLE city_temperatures "
      "     FROM 'csv:test/fixtures/city_temperatures.csv?headers=true';"
      ""
      "  SELECT city FROM city_temperatures WHERE city >= 'New York'"
      "     GROUP BY city LIMIT 10;");

  TableRepository table_repo;
  QueryPlan query_plan(&table_repo);
  runtime.queryPlanBuilder()->buildQueryPlan(ast, &query_plan);

  ResultList result;
  auto query_plan_node = query_plan.queries()[0].get();
  query_plan_node->setTarget(&result);
  query_plan_node->execute();

  EXPECT(result.getNumRows() == 2);
  EXPECT(result.getRow(0)[0] == "New York");
  EXPECT(result.getRow(1)[0] == "Tokyo");
});

TEST_CASE(SQLTest, TestSimpleGroupOverTimeWindow, [] () {
  auto result = executeTestQuery(
      "  SELECT time as X, sum(value) as Y"
      "      FROM timeseries"
      "      GROUP OVER TIMEWINDOW(time, 60, 20);");

  EXPECT_EQ(result->getNumRows(), 29);
  EXPECT_EQ(result->getNumColumns(), 2);
  EXPECT_EQ(result->getColumns()[0], "X");
  EXPECT_EQ(result->getColumns()[1], "Y");
  EXPECT_EQ(result->getRow(0)[1], "1770");
  EXPECT_EQ(result->getRow(10)[1], "13770");
  EXPECT_EQ(result->getRow(15)[1], "NULL");
  EXPECT_EQ(result->getRow(28)[1], "28170");
});

TEST_CASE(SQLTest, TestNumericConversion, [] () {
  {
    SValue val("42");
    EXPECT_EQ(val.getType(), SValue::T_STRING);
    EXPECT(val.testType<SValue::IntegerType>());
    EXPECT(val.tryNumericConversion());
    EXPECT_EQ(val.getInteger(), 42);
  }

  {
    SValue val("1415912541648");
    EXPECT_EQ(val.getType(), SValue::T_STRING);
    EXPECT(val.testType<SValue::IntegerType>());
    EXPECT(val.tryNumericConversion());
    EXPECT_EQ(val.getInteger(), 1415912541648lu);
  }
});

TEST_CASE(SQLTest, TestCompareTimestamps, [] () {
  auto result = executeTestQuery(
      "  SELECT"
      "  FROM_TIMESTAMP(1415916005281) > FROM_TIMESTAMP(1415916005281) as a,"
      "  FROM_TIMESTAMP(1415916005281) < FROM_TIMESTAMP(1415916005281) as b,"
      "  FROM_TIMESTAMP(1415916005282) > FROM_TIMESTAMP(1415916005281) as c,"
      "  FROM_TIMESTAMP(1415916005280) < FROM_TIMESTAMP(1415916005281) as d,"
      "  FROM_TIMESTAMP(1415916005280) > FROM_TIMESTAMP(1415916005281) as e,"
      "  FROM_TIMESTAMP(1415916005282) < FROM_TIMESTAMP(1415916005281) as f;");

  EXPECT_EQ(result->getNumRows(), 1);
  EXPECT_EQ(result->getNumColumns(), 6);
  EXPECT_EQ(result->getRow(0)[0].size(), std::string("false").size());
  EXPECT_EQ(result->getRow(0)[1], std::string("false"));
  EXPECT_EQ(result->getRow(0)[2], std::string("true"));
  EXPECT_EQ(result->getRow(0)[3], std::string("true"));
  EXPECT_EQ(result->getRow(0)[4], std::string("false"));
  EXPECT_EQ(result->getRow(0)[5], std::string("false"));
});

TEST_CASE(SQLTest, TestInvalidQueries, [] () {
  std::vector<std::string> queries;

  queries.push_back(
      "SELECT time AS x, count(value) as y FROM `http_status_codes`"
      "where GROUP OVER TIMEWINDOW(time, 1000, 1) BY test;");

  for (const auto& query : queries) {
    bool raised = false;

    try {
      executeTestQuery(query.c_str());
    } catch (std::exception& e) {
      raised = true;
    }

    EXPECT(raised);
  }
});

TEST_CASE(SQLTest, TestMeanAggregation, [] () {
  auto results = executeTestQuery(
      "  SELECT"
      "    mean(one)"
      "  FROM"
      "    testtable2;");

  EXPECT_EQ(results->getNumRows(), 1);
  EXPECT_EQ(results->getRow(0)[0], "6.875000");
});

TEST_CASE(SQLTest, TestMaxAggregation, [] () {
  auto results = executeTestQuery(
      "  SELECT"
      "    max(one)"
      "  FROM"
      "    testtable2;");

  EXPECT_EQ(results->getNumRows(), 1);
  EXPECT_EQ(results->getRow(0)[0], "10.000000");
});

TEST_CASE(SQLTest, TestMinAggregation, [] () {
  auto results = executeTestQuery(
      "  SELECT"
      "    min(one)"
      "  FROM"
      "    testtable2;");

  EXPECT_EQ(results->getNumRows(), 1);
  EXPECT_EQ(results->getRow(0)[0], "1.000000");
});

