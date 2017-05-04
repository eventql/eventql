/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#include <unistd.h>
#include "eventql/util/io/fileutil.h"
#include "basic_sql.h"
#include "../../automate/cluster.h"
#include "../../automate/process.h"
#include "../../automate/query.h"
#include "../../automate/tables.h"
#include "../../test_runner.h"

namespace eventql {
namespace test {
namespace system_basic_sql {

static void setup_tests(TestBundle* test_bundle, const std::string& id_prefix) {
  {
    TestCase t;
    t.test_id = id_prefix + "002";
    t.description = "Create table: pageviews";
    t.suites = std::set<TestSuite> { TestSuite::WORLD, TestSuite::SMOKE };
    t.fun = [] (TestContext* ctx) -> bool {
      executeQueryAndExpectSuccess(
          ctx,
          "./test/system/basic_sql/create_pageviews.sql",
          "localhost",
          "9175",
          "test");

      return true;
    };
    test_bundle->test_cases.emplace_back(t);
  }

  {
    TestCase t;
    t.test_id = id_prefix + "003";
    t.description = "Create table: customers";
    t.suites = std::set<TestSuite> { TestSuite::WORLD, TestSuite::SMOKE };
    t.fun = [] (TestContext* ctx) -> bool {
      executeQueryAndExpectSuccess(
          ctx,
          "./test/system/basic_sql/create_customers.sql",
          "localhost",
          "9175",
          "test");

      return true;
    };
    test_bundle->test_cases.emplace_back(t);
  }

  {
    TestCase t;
    t.test_id = id_prefix + "004";
    t.description = "Import table: customers";
    t.suites = std::set<TestSuite> { TestSuite::WORLD, TestSuite::SMOKE };
    t.fun = [] (TestContext* ctx) -> bool {
      ImportTableOpts opts;
      opts.host = "localhost";
      opts.port = "9175";
      opts.database = "test";
      opts.table = "customers";
      opts.input_file = "test/sql_testdata/testtbl2.csv";

      importTable(ctx, opts);
      return true;
    };
    test_bundle->test_cases.emplace_back(t);
  }
}

void setup_tests(TestRepository* test_repo) {
  // standalone cluster
  {
    TestBundle t;
    t.logfile_path = "test/system/basic_sql/test.log";

    TestCase tc;
    tc.test_id = "SYS-BASICSQL-STANDALONE-001";
    tc.description = "Start & create cluster";
    tc.suites = std::set<TestSuite> { TestSuite::WORLD, TestSuite::SMOKE };
    tc.fun = [] (TestContext* ctx) -> bool {
      startStandaloneCluster(ctx);
      return true;
    };
    t.test_cases.emplace_back(tc);

    setup_tests(&t, "SYS-BASICSQL-STANDALONE-");
    test_repo->addTestBundle(t);
  }
}

} // namespace unit
} // namespace test
} // namespace eventql

