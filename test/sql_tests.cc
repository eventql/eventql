/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Laura Schlimmer <laura@eventql.io>
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
#include "eventql/eventql.h"
#include "eventql/util/stringutil.h"
#include "test_repository.h"

namespace eventql {
namespace test {
namespace sql {

bool runTest(std::string id) {

  return false;
}

void setupTestCase(std::string id, std::vector<TestCase>* test_bundle) {
  auto test_case = eventql::test::TestCase {
    .test_id = id,
    .description = StringUtil::format("Test-$0", id),
    .fun = std::bind(runTest, id),
    .suites =  { TestSuite::WORLD, TestSuite::SMOKE }
  };

  test_bundle->emplace_back(test_case);
}

void setup_sql_tests(TestRepository* repo) {
  std::vector<TestCase> test_bundle;
  setupTestCase("00001", &test_bundle);

  repo->addTestBundle(test_bundle);

 // SETUP_UNIT_TEST(metadata_store, repo);
 // SETUP_UNIT_TEST(metadata_file, repo);
 // SETUP_UNIT_TEST(metadata_operation_createpartition, repo);
 // SETUP_UNIT_TEST(metadata_operation_split, repo);
}

} // namespace sql
} // namespace test
} // namespace eventql


