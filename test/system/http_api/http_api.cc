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
#include "http_api.h"
#include "../../automate/cluster.h"
#include "../../automate/curl.h"
#include "../../test_runner.h"

namespace eventql {
namespace test {
namespace system_http_api {

void setup_tests(TestRepository* test_repo) {
// standalone cluster
  {
    TestBundle t;
    t.logfile_path = "test/system/http_api/test.log";

    TestCase tc;
    tc.test_id = "SYS-HTTPAPI-STANDALONE-001";
    tc.description = "/api/v1/tables/insert";
    tc.suites = std::set<TestSuite> { TestSuite::WORLD, TestSuite::SMOKE };
    tc.fun = [] (TestContext* ctx) -> bool {
      startStandaloneCluster(ctx);

      executePOST(
          ctx,
          "test/system/http_api/test",
          "tables/insert",
          "localhost",
          "9175",
          "[{\"table\": \"customers\", \"database\": \"test\", \"data\": {\"customerid\": 123}}]");

      return true;
    };
    t.test_cases.emplace_back(tc);

   // setup_tests(&t, "SYS-BASICSQL-STANDALONE-");
    test_repo->addTestBundle(t);
  }
}

} // namespace system_http_api
} // namespace test
} // namespace eventql

