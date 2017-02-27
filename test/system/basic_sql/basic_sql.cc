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
#include "basic_sql.h"
#include "../../automate/process.h"
#include "../../automate/automate_query.h"
#include "../../test_runner.h"

namespace eventql {
namespace test {
namespace system_basic_sql {

static bool init_cluster_standalone(TestContext* ctx) {
  std::string datadir = "/tmp/__evql_test_" + Random::singleton()->hex64();
  FileUtil::mkdir_p(datadir);

  std::unique_ptr<Process> evqld_proc(new Process());
  evqld_proc->logDebug("evqld-standalone", ctx->log_fd);
  evqld_proc->start(
      FileUtil::joinPaths(ctx->bindir, "evqld"),
      std::vector<std::string> {
        "--standalone",
        "--datadir", datadir
      });

  ctx->background_procs["evqld-standalone"] = std::move(evqld_proc);
  usleep(500000); // FIXME give the process time to start
  return true;
}

static bool init_cluster_tables(TestContext* ctx) {
  std::set<std::string> tables{ "pageviews", "customers" };
  for (const auto& tbl: tables) {
    Process::runOrDie(
        FileUtil::joinPaths(ctx->bindir, "evql"),
        {
          "-d", "test",
          "-f", "./test/system/basic_sql/create_" + tbl + ".sql"
        },
        {},
        "evql-create-table",
        ctx->log_fd);
  }

  return true;
}

static bool import_cluster_data(TestContext* ctx) {
  Process::runOrDie(
      FileUtil::joinPaths(ctx->bindir, "evqlctl"),
      {
        "table-import",
        "--host", "localhost",
        "--port", "9175",
        "--database", "test",
        "--table", "customers",
        "--format", "csv",
        "--file", FileUtil::joinPaths(ctx->srcdir, "test/sql_testdata/testtbl2.csv")
      },
      {},
      "evql-import-customers-table",
      ctx->log_fd);

  return true;
}

void setup_tests(TestRepository* test_repo) {
  // standalone cluster
  {
    TestBundle t;
    t.logfile_path = "test/system/basic_sql/test.log";

    t.test_cases.emplace_back(TestCase {
      .test_id = "SYS-BASICSQL-STANDALONE-001",
      .description = "Start & create cluster",
      .fun = &init_cluster_standalone,
      .suites = std::set<TestSuite> { TestSuite::WORLD, TestSuite::SMOKE }
    });
    t.test_cases.emplace_back(TestCase {
      .test_id = "SYS-BASICSQL-STANDALONE-002",
      .description = "Create tables",
      .fun = &init_cluster_tables,
      .suites = std::set<TestSuite> { TestSuite::WORLD, TestSuite::SMOKE }
    });
    t.test_cases.emplace_back(TestCase {
      .test_id = "SYS-BASICSQL-STANDALONE-003",
      .description = "Import table data from CSV",
      .fun = &import_cluster_data,
      .suites = std::set<TestSuite> { TestSuite::WORLD, TestSuite::SMOKE }
    });
    test_repo->addTestBundle(t);
  }
}

} // namespace unit
} // namespace test
} // namespace eventql

