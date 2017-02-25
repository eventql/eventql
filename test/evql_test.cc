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
#include <iostream>
#include "eventql/eventql.h"
#include "eventql/util/cli/flagparser.h"
#include "test_repository.h"
#include "test_runner.h"
#include "regress/basic_sql/basic_sql.h"

using namespace eventql::test;

const TestOutputFormat kDefaultTestOutputFormat = TestOutputFormat::ASCII;

namespace eventql {
namespace test {
namespace unit {
extern void setup_unit_tests(TestRepository* repo);
} // namespace unit
} // namespace test
} // namespace eventql

int main(int argc, const char** argv) {
  cli::FlagParser flags;

  flags.defineFlag(
      "help",
      cli::FlagParser::T_SWITCH,
      false,
      "?",
      NULL,
      "help",
      "<help>");

  flags.defineFlag(
      "list",
      cli::FlagParser::T_SWITCH,
      false,
      "l",
      NULL,
      "list",
      "<switch>");

  flags.defineFlag(
      "test",
      cli::FlagParser::T_STRING,
      false,
      "t",
      NULL,
      "test",
      "<test_id>");

  flags.defineFlag(
      "suite",
      cli::FlagParser::T_STRING,
      false,
      "s",
      NULL,
      "suite",
      "<suite_id>");

  flags.defineFlag(
      "output",
      cli::FlagParser::T_STRING,
      false,
      "o",
      NULL,
      "output",
      "<format>");

  flags.parseArgv(argc, argv);

  if (flags.isSet("help")) {
    std::cout
      << StringUtil::format(
            "EventQL $0 ($1)\n"
            "Copyright (c) 2016, DeepCortex GmbH. All rights reserved.\n\n",
            eventql::kVersionString,
            eventql::kBuildID);

    std::cout
        << "Usage: $ evql-test [OPTIONS]" << std::endl
        <<  "   -l, --list                List all tests" << std::endl
        <<  "   -t, --test <test_id>      Run a single specific test" << std::endl
        <<  "   -s, --suite <suite>       Run a test suite (world, smoke)" << std::endl
        <<  "   -o, --output <format>     Output format (ascii, tap)" << std::endl
        <<  "   -?, --help                Display this help text and exit" <<std::endl;
    return 0;
  }

  auto output_format = kDefaultTestOutputFormat;
  if (flags.isSet("output")) {
    auto output_format_str = flags.getString("output");
    if (output_format_str == "ascii") {
      output_format = TestOutputFormat::ASCII;
    } else if (output_format_str == "tap") {
      output_format = TestOutputFormat::TAP;
    } else {
      std::cerr << "ERROR: invalid output format: " << output_format_str << std::endl;
      return 1;
    }
  }

  if (!(flags.isSet("list") ^ flags.isSet("test") ^ flags.isSet("suite"))) {
    std::cerr << "ERROR: exactly one of the --list, --test or --suite flags must be set" << std::endl;
    return 1;
  }

  /* init test repo */
  TestRepository test_repo;
  eventql::test::unit::setup_unit_tests(&test_repo);
  eventql::test::regress_basic_sql::setup_tests(&test_repo);

  /* run tests */
  TestRunner test_runner(&test_repo);

  if (flags.isSet("list")) {
    return 0;
  }

  auto test_result = false;
  if (flags.isSet("test")) {
    test_result = test_runner.runTest(flags.getString("test"), output_format);
  }

  if (flags.isSet("suite")) {
    test_result = test_runner.runTestSuite(
        flags.getString("suite"),
        output_format);
  }

  return test_result ? 0 : 1;
}

