/**
 * Copyright (c) 2017 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#include "eventql/util/stringutil.h"
#include "test_runner.h"

namespace eventql {
namespace test {

TestRunner::TestRunner(TestRepository* test_repo) : test_repo_(test_repo) {}

bool TestRunner::runTest(const std::string& test_id, TestOutputFormat format) {
  return runTests(std::set<std::string>{test_id}, format);
}

static void expandTestList(
    TestRepository* test_repo,
    std::set<std::string>* test_ids) {
}

bool TestRunner::runTests(
    std::set<std::string> test_ids,
    TestOutputFormat format) {
  expandTestList(test_repo_, &test_ids);

  size_t tests_count = test_ids.size();

  switch (format) {
    case TestOutputFormat::ASCII:
      fprintf(
          stderr,
          "\033[1;97mRunning %i/%i tests\e[0m\n\n",
          (int) tests_count,
          (int) test_repo_->getTestCount());
      break;
    case TestOutputFormat::TAP:
      fprintf(stdout, "1..%i\n", (int) tests_count);
      break;
  }

  std::vector<std::pair<std::string, std::string>> test_failures;
  size_t tests_passed = 0;
  size_t test_num = 0;
  for (const auto& bundle : test_repo_->getTestBundles()) {
    for (const auto& test : bundle) {
      if (test_ids.count(test.test_id) == 0) {
        break;
      }

      ++test_num;

      bool test_result = false;
      std::string test_message;
      try {
        test_result = test.fun();
      } catch (const std::exception& e) {
        test_message = e.what();
      }

      StringUtil::replaceAll(&test_message, "\n", "\\n");
      StringUtil::replaceAll(&test_message, "\r", "");

      if (test_result) {
        ++tests_passed;

        switch (format) {
          case TestOutputFormat::ASCII:
            fprintf(
                stderr,
                " * %s \033[1;32m[PASS]\e[0m\n",
                test.test_id.c_str());
            break;
          case TestOutputFormat::TAP:
            fprintf(
                stdout,
                "ok %i - %s\n",
                (int) test_num,
                test.test_id.c_str());
            break;
        }
      } else {
        test_failures.emplace_back(test.test_id, test_message);

        switch (format) {
          case TestOutputFormat::ASCII:
            fprintf(
                stderr,
                " * %s \033[1;31m[FAIL]\e[0m\n",
                test.test_id.c_str());
            break;
          case TestOutputFormat::TAP:
            fprintf(
                stdout,
                "not ok %i - %s # %s\n",
                (int) test_num,
                test.test_id.c_str(),
                test_message.c_str());
            break;
        }
      }
    }
  }

  if (tests_count == 0 || tests_passed < tests_count) {
    switch (format) {
      case TestOutputFormat::ASCII:
        for (const auto& fail : test_failures) {
          fprintf(
              stderr,
              "\n\033[1;31m[FAIL] %s\e[0m\n  %s\n",
              fail.first.c_str(),
              fail.second.c_str());
        }

        fprintf(
            stderr,
            "\n\033[1;31m[FAIL] %i/%i tests failed :(\e[0m\n",
            (int) (tests_count - tests_passed),
            (int) tests_count);

        break;
      case TestOutputFormat::TAP:
        break;
    }

    return false;
  } else {
    switch (format) {
      case TestOutputFormat::ASCII:
        fprintf(
            stderr,
            "\n\033[1;32m[PASS] %i tests passed :)\e[0m\n",
            (int) tests_count);
        break;
      case TestOutputFormat::TAP:
        break;
    }

    return true;
  }
}

bool TestRunner::runTestSuite(const std::string& suite, TestOutputFormat format) {
  if (suite == "world") {
    return runTestSuite(TestSuite::WORLD, format);
  }

  if (suite == "smoke") {
    return runTestSuite(TestSuite::WORLD, format);
  }

  std::cerr << "ERROR: invalid test suite" << std::endl;
  return false;
}

bool TestRunner::runTestSuite(TestSuite suite, TestOutputFormat format) {
  std::set<std::string> test_ids;

  for (const auto& bundle : test_repo_->getTestBundles()) {
    for (const auto& test : bundle) {
      if (test.suites.count(suite) > 0) {
        test_ids.insert(test.test_id);
      }
    }
  }

  return runTests(test_ids, format);
}

void TestRunner::printTestList() {
}

} // namespace test
} // namespace eventql

