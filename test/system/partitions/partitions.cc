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
#include "eventql/util/random.h"
#include "eventql/util/SHA1.h"
#include "eventql/util/UnixTime.h"
#include "eventql/util/csv/CSVOutputStream.h"
#include "partitions.h"
#include "../../automate/cluster.h"
#include "../../automate/query.h"
#include "../../automate/tables.h"
#include "../../test_runner.h"

namespace eventql {
namespace test {
namespace system_partitions {

static void generate_pageviews_csv(const std::string& file, size_t row_count) {
  if (FileUtil::exists(file)) {
    return;
  }

  CSVOutputStream os(FileOutputStream::openFile(file), ",");

  os.appendRow({
    "time",
    "sid",
    "request_id",
    "url",
    "user_agent",
    "referrer",
    "product_id",
    "time_on_page",
    "screen_width",
    "screen_height",
    "is_logged_in"
  });

  uint64_t time_epoch = 1488211057000000;

  for (size_t i = 0; i < row_count; ++i) {
    os.appendRow({
      UnixTime(time_epoch + i * 250000).toString(),        // time
      std::to_string(i % 100),                             // sid
      std::to_string(i),                                   // request_id
      "/",                                                 // url
      "MyUserAgent",                                       // user_agent
      "http://example.com",                                // referrer
      "1234",                                              // product_id
      "1.42",                                              // time_on_page
      "1440",                                              // screen_width
      "900",                                               // screen_height
      "false"                                              // is_logged_in
    });
  }
}

static void setup_tests(TestBundle* test_bundle, const std::string& id_prefix) {
  {
    TestCase t;
    t.test_id = id_prefix + "002";
    t.description = "Create table: pageviews_with_pkey";
    t.suites = std::set<TestSuite> { TestSuite::WORLD, TestSuite::SMOKE };
    t.fun = [] (TestContext* ctx) -> bool {
      executeQueryAndExpectSuccess(
          ctx,
          "./test/system/partitions/create_pageviews_with_pkey.sql",
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
    t.description = "Create table: pageviews_without_pkey";
    t.suites = std::set<TestSuite> { TestSuite::WORLD, TestSuite::SMOKE };
    t.fun = [] (TestContext* ctx) -> bool {
      executeQueryAndExpectSuccess(
          ctx,
          "./test/system/partitions/create_pageviews_without_pkey.sql",
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
    t.description = "Import table data from CSV: pageviews_with_pkey";
    t.suites = std::set<TestSuite> { TestSuite::WORLD, TestSuite::SMOKE };
    t.fun = [] (TestContext* ctx) -> bool {
      generate_pageviews_csv(
          "test/system/partitions/pageviews_10m.gen.csv",
          10000000);

      ImportTableOpts opts;
      opts.host = "localhost";
      opts.port = "9175";
      opts.database = "test";
      opts.table = "pageviews_with_pkey";
      opts.input_file = "test/system/partitions/pageviews_10m.gen.csv";

      importTable(ctx, opts);
      return true;
    };
    test_bundle->test_cases.emplace_back(t);
  }

  {
    TestCase t;
    t.test_id = id_prefix + "005";
    t.description = "Test: test_count_pageviews_with_pkey.sql";
    t.suites = std::set<TestSuite> { TestSuite::WORLD, TestSuite::SMOKE };
    t.fun = [] (TestContext* ctx) -> bool {
      executeTestQueryWithRetries(
          ctx,
          "test/system/partitions/test_count_pageviews_with_pkey.sql",
          "localhost",
          "9175",
          "test",
          300);

      return true;
    };
    test_bundle->test_cases.emplace_back(t);
  }

  {
    TestCase t;
    t.test_id = id_prefix + "006";
    t.description = "Test: test_select_row_1337_from_pageviews_with_pkey.sql";
    t.suites = std::set<TestSuite> { TestSuite::WORLD, TestSuite::SMOKE };
    t.fun = [] (TestContext* ctx) -> bool {
      executeTestQuery(
          ctx,
          "test/system/partitions/test_select_row_1337_from_pageviews_with_pkey.sql",
          "localhost",
          "9175",
          "test");

      return true;
    };
    test_bundle->test_cases.emplace_back(t);
  }

  {
    TestCase t;
    t.test_id = id_prefix + "007";
    t.description = "Test: test_update_row_1337_in_pageviews_with_pkey.sql";
    t.suites = std::set<TestSuite> { TestSuite::WORLD, TestSuite::SMOKE };
    t.fun = [] (TestContext* ctx) -> bool {
      executeQueryAndExpectSuccess(
          ctx,
          "test/system/partitions/test_update_row_1337_in_pageviews_with_pkey.sql",
          "localhost",
          "9175",
          "test");

      return true;
    };
    test_bundle->test_cases.emplace_back(t);
  }

  {
    TestCase t;
    t.test_id = id_prefix + "008";
    t.description = "Test: test_count_pageviews_with_pkey.sql";
    t.suites = std::set<TestSuite> { TestSuite::WORLD, TestSuite::SMOKE };
    t.fun = [] (TestContext* ctx) -> bool {
      executeTestQueryWithRetries(
          ctx,
          "test/system/partitions/test_count_pageviews_with_pkey.sql",
          "localhost",
          "9175",
          "test",
          300);

      return true;
    };
    test_bundle->test_cases.emplace_back(t);
  }

  {
    TestCase t;
    t.test_id = id_prefix + "009";
    t.description = "Test: test_select_row_1337_from_pageviews_with_pkey_after_update.sql";
    t.suites = std::set<TestSuite> { TestSuite::WORLD, TestSuite::SMOKE };
    t.fun = [] (TestContext* ctx) -> bool {
      executeTestQuery(
          ctx,
          "test/system/partitions/test_select_row_1337_from_pageviews_with_pkey_after_update.sql",
          "localhost",
          "9175",
          "test");

      return true;
    };
    test_bundle->test_cases.emplace_back(t);
  }

  {
    TestCase t;
    t.test_id = id_prefix + "010";
    t.description = "test: test_insert_into_pageviews_with_pkey.sql";
    t.suites = std::set<TestSuite> { TestSuite::WORLD, TestSuite::SMOKE };
    t.fun = [] (TestContext* ctx) -> bool {
      executeQueryAndExpectSuccess(
          ctx,
          "test/system/partitions/test_insert_into_pageviews_with_pkey.sql",
          "localhost",
          "9175",
          "test");

      return true;
    };
    test_bundle->test_cases.emplace_back(t);
  }

  {
    TestCase t;
    t.test_id = id_prefix + "011";
    t.description = "Test: test_select_new_row_from_pageviews_with_pkey.sql";
    t.suites = std::set<TestSuite> { TestSuite::WORLD, TestSuite::SMOKE };
    t.fun = [] (TestContext* ctx) -> bool {
      executeTestQuery(
          ctx,
          "test/system/partitions/test_select_new_row_from_pageviews_with_pkey.sql",
          "localhost",
          "9175",
          "test");

      return true;
    };
    test_bundle->test_cases.emplace_back(t);
  }

  {
    TestCase t;
    t.test_id = id_prefix + "012";
    t.description = "test: test_count_pageviews_with_pkey_after_insert.sql";
    t.suites = std::set<TestSuite> { TestSuite::WORLD, TestSuite::SMOKE };
    t.fun = [] (TestContext* ctx) -> bool {
      executeTestQueryWithRetries(
          ctx,
          "test/system/partitions/test_count_pageviews_with_pkey_after_insert.sql",
          "localhost",
          "9175",
          "test",
          300);

      return true;
    };
    test_bundle->test_cases.emplace_back(t);
  }
}

void setup_tests(TestRepository* test_repo) {
  // standalone cluster
  {
    TestBundle t;
    t.logfile_path = "test/system/partitions/test.log";

    TestCase tc;
    tc.test_id = "SYS-PARTITIONS-STANDALONE-001";
    tc.description = "Start & create cluster";
    tc.suites = std::set<TestSuite> { TestSuite::WORLD, TestSuite::SMOKE };
    tc.fun = [] (TestContext* ctx) -> bool {
      startStandaloneCluster(ctx);
      return true;
    };
    t.test_cases.emplace_back(tc);

    setup_tests(&t, "SYS-PARTITIONS-STANDALONE-");
    test_repo->addTestBundle(t);
  }
}

} // namespace unit
} // namespace test
} // namespace eventql

