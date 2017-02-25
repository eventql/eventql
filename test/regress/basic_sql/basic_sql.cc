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
#include "basic_sql.h"

namespace eventql {
namespace test {
namespace regress_basic_sql {

static bool init_cluster_standalone(TestContext* ctx) {

  return true;
}

void setup_tests(TestRepository* test_repo) {
  // standalone cluster
  {
    std::vector<TestCase> cases;
    cases.emplace_back(TestCase {
      .test_id = "REGRESS-BASICSQL-STANDALONE-001",
      .description = "Start & create cluster",
      .fun = &init_cluster_standalone,
      .suites = std::set<TestSuite> { TestSuite::WORLD, TestSuite::SMOKE }
    });
    test_repo->addTestBundle(cases);
  }
}

} // namespace unit
} // namespace test
} // namespace eventql

