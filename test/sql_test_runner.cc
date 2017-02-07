/**
 * Copyright (c) 2017 DeepCortex GmbH <legal@eventql.io>
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
#include <iostream>
#include "eventql/util/stdtypes.h"
#include "eventql/util/exception.h"
#include "eventql/util/status.h"
#include "eventql/util/stringutil.h"
#include "eventql/util/io/fileutil.h"
#include "eventql/util/io/inputstream.h"
#include "eventql/util/io/TerminalOutputStream.h"
#include "eventql/util/cli/flagparser.h"
#include "eventql/util/csv/CSVInputStream.h"
#include "eventql/util/csv/CSVOutputStream.h"
#include "eventql/sql/runtime/defaultruntime.h"
#include "eventql/sql/CSTableScanProvider.h"
#include "eventql/sql/result_list.h"

#include "eventql/eventql.h"
using namespace csql;

enum class ResultExpectation { TABLE, CHART, ERROR };

enum class OutputFormat { TAP, CSV, VERBOSE };

const auto kTestListFile = "./sql_tests.lst";
const auto kDirectoryPath = "./sql";
const auto kSQLPathEnding = ".sql";
const auto kResultPathEnding = ".result.txt";
const auto kChartColumnName = "__chart";
const auto kDefaultResultExpectation = ResultExpectation::TABLE;
const auto kDefaultOutputFormat = OutputFormat::TAP;

static void printError(const std::string& error_string) {
  auto stderr_os = TerminalOutputStream::fromStream(OutputStream::getStderr());
  stderr_os->print(
      "ERROR:",
      { TerminalStyle::RED, TerminalStyle::UNDERSCORE });
  stderr_os->print(" " + error_string + "\n");
}

std::string getResultCSV(ResultList* result, const std::string& row_sep = "\n") {
  std::string result_csv;
  auto is = new StringOutputStream(&result_csv);
  auto csv_is = new CSVOutputStream(
      std::unique_ptr<OutputStream>(is),
      ";",
      row_sep);

  auto columns = result->getColumns();
  csv_is->appendRow(columns);

  auto num_rows = result->getNumRows();
  for (size_t i = 0; i < num_rows; ++i) {
    auto row = result->getRow(i);
    csv_is->appendRow(row);
  }

  return result_csv;
}

Status checkTableResult(
    ResultList* result,
    const std::string& result_file_path) {
  auto csv_is = CSVInputStream::openFile(result_file_path);
  std::vector<std::string> columns;
  if (!csv_is->readNextRow(&columns)) {
    return Status(eRuntimeError, "CSV needs a header");
  }

  /* compare columns */
  if (result->getNumColumns() != columns.size()) {
    return Status(eRuntimeError, StringUtil::format(
        "wrong number of columns, expected $0 to be $1",
        result->getNumColumns(),
        columns.size()));
  }

  auto returned_columns = result->getColumns();
  for (size_t i = 0; i < returned_columns.size(); ++i) {
    if (returned_columns[i] != columns[i]) {
      return Status(eRuntimeError, StringUtil::format(
          "wrong columns name, expected $0 to be $1",
          returned_columns[i],
          columns[i]));
    }
  }

  /* compare rows */
  auto num_returned_rows = result->getNumRows();
  size_t count = 0;
  std::vector<std::string> row;
  while (csv_is->readNextRow(&row)) {
    if (count >= num_returned_rows) {
      return Status(eRuntimeError, "not enough rows returned");
    }

    auto returned_row = result->getRow(count);
    if (returned_row.size() != row.size()) {
      return Status(eRuntimeError, StringUtil::format(
          "wrong number of values, expected $0 to be $1",
          returned_row.size(),
          row.size()));
    }

    for (size_t i = 0; i < row.size(); ++i) {
      if (row[i] != returned_row[i]) {
        return Status(
            eRuntimeError,
            StringUtil::format(
                "result mismatch at row $0, column $1; expected: >$2<, got: >$3<",
                count,
                i,
                row[i],
                returned_row[i]));
      }
    }

    ++count;
  }

  if (count < num_returned_rows) {
    return Status(eRuntimeError, StringUtil::format(
        "too many rows, expected $0 to be $1",
        num_returned_rows,
        count));
  }

  return Status::success();
}

Status checkChartResult(
    ResultList* result,
    const std::string& result_file_path) {
  auto num_returned_rows = result->getNumRows();
  if (num_returned_rows != 1) {
    return Status(eRuntimeError, StringUtil::format(
        "wrong number of rows returned, expected $0 to be 1",
        num_returned_rows));
  }

  auto is = FileInputStream::openFile(result_file_path);
  std::string expected_result;
  is->readUntilEOF(&expected_result);

  auto returned_result = result->getRow(0)[0];
  if (expected_result != returned_result) {
    return Status(eRuntimeError, "result charts don't match");
  }

  return Status::success();
}

Status checkErrorResult(
    const std::string& error_message,
    ResultList* result,
    const std::string& result_file_path) {
  auto is = FileInputStream::openFile(result_file_path);

  {
    std::string result_expectation_line;
    is->readLine(&result_expectation_line);
  }

  std::string expected_error;
  is->readUntilEOF(&expected_error);
  StringUtil::chomp(&expected_error);

  if (expected_error == error_message) {
    return Status::success();
  } else {
    if (error_message.empty()) {
      auto result_csv = getResultCSV(result, " ");
      return Status(eRuntimeError, StringUtil::format(
          "expected error: $0 but got result: $1",
          expected_error,
          result_csv));

    } else {
      return Status(eRuntimeError, StringUtil::format(
          "expected error: $0 but got error $1",
          expected_error,
          error_message));
    }
  }
}

Status runTest(const std::string& test, OutputFormat output_format) {
  auto sql_file_path = FileUtil::joinPaths(
      kDirectoryPath,
      StringUtil::format("$0$1", test, kSQLPathEnding));

  if (!FileUtil::exists(sql_file_path)) {
    return Status(
        eIOError,
        StringUtil::format("File does not exist: $0", sql_file_path));
  }

  auto result_file_path = FileUtil::joinPaths(
      kDirectoryPath,
      StringUtil::format("$0$1", test, kResultPathEnding));

  if (!FileUtil::exists(result_file_path)) {
    return Status(
        eIOError,
        StringUtil::format("File does not exist: $0", result_file_path));
  }

  auto result_expectation = kDefaultResultExpectation;
  auto result_is = FileInputStream::openFile(result_file_path);
  std::string first_line;
  if (result_is->readLine(&first_line)) {
    StringUtil::chomp(&first_line);
    if (first_line == "ERROR!") {
      result_expectation = ResultExpectation::ERROR;
    }
  }

  /* input table path */
  auto sql_is = FileInputStream::openFile(sql_file_path);
  std::string input_table_path;
  if (!sql_is->readLine(&input_table_path) ||
      !StringUtil::beginsWith(input_table_path, "--")) {
    return Status(eRuntimeError, "no input table provided");
  }

  StringUtil::replaceAll(&input_table_path, "--");
  StringUtil::ltrim(&input_table_path);
  StringUtil::chomp(&input_table_path);

  if (!FileUtil::exists(input_table_path)) {
    return Status(
        eRuntimeError,
        StringUtil::format("file does not exist: $0", input_table_path));
  }

  std::string query;
  sql_is->readUntilEOF(&query);

  /* execute query */
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  txn->setTableProvider(new CSTableScanProvider("testtable", input_table_path));
  ResultList result;

  std::string error_message;
  auto rc = Status::success();
  try {
    auto qplan = runtime->buildQueryPlan(txn.get(), query);
    qplan->execute(0, &result);
  } catch (const std::exception& e) {
    error_message = e.what();

    if (result_expectation != ResultExpectation::ERROR) {
      return Status(
          eRuntimeError,
          StringUtil::format("unexpected error: $0", error_message));
    }
  }

  if (result.getNumColumns() == 1 &&
      result.getColumns()[0] == kChartColumnName) {
    result_expectation = ResultExpectation::CHART;
  }

  switch (result_expectation) {
    case ResultExpectation::TABLE:
      rc = checkTableResult(&result, result_file_path);
      break;

    case ResultExpectation::CHART:
      rc = checkChartResult(&result, result_file_path);
      break;

    case ResultExpectation::ERROR:
      rc = checkErrorResult(error_message, &result, result_file_path);
      break;
  }

  if (rc.isSuccess()) {
    return rc;
  }

  switch (output_format) {
    case OutputFormat::CSV:
      return Status(eRuntimeError, getResultCSV(&result));

    case OutputFormat::VERBOSE: {
      std::string output = StringUtil::format("$0\n", rc.message());

      /* add the expected result's debug print */
      {
        ResultList expected_result;
        expected_result.addHeader(StringUtil::split(first_line, ";"));
        std::string line;
        while (result_is->readLine(&line)) {
          StringUtil::chomp(&line);
          expected_result.addRow(StringUtil::split(line, ";"));
          line.clear();
        }

        output.append("expected:\n");
        auto string_is = new StringOutputStream(&output);
        expected_result.debugPrint(string_is);
      }

      /* add the returned result's debug print */
      {
        output.append("got:\n");
        auto string_is = new StringOutputStream(&output);
        result.debugPrint(string_is);
      }

      return Status(eRuntimeError, output);
    }

    case OutputFormat::TAP:
      return rc;
  }
}

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
      "test",
      cli::FlagParser::T_STRING,
      false,
      "t",
      NULL,
      "test",
      "<test_number>");

  flags.defineFlag(
      "tap",
      cli::FlagParser::T_SWITCH,
      false,
      NULL,
      NULL,
      "tap",
      "<tap>");

  flags.defineFlag(
      "csv",
      cli::FlagParser::T_SWITCH,
      false,
      NULL,
      NULL,
      "csv",
      "<csv>");

  flags.defineFlag(
      "verbose",
      cli::FlagParser::T_SWITCH,
      false,
      NULL,
      NULL,
      "verbose",
      "<verbose>");

  flags.parseArgv(argc, argv);

  if (flags.isSet("help")) {
    std::cout
      << StringUtil::format(
            "EventQL $0 ($1)\n"
            "Copyright (c) 2016, DeepCortex GmbH. All rights reserved.\n\n",
            eventql::kVersionString,
            eventql::kBuildID);

    std::cout
      << "Usage: $ evql-test-sql [OPTIONS]" << std::endl
      <<  "   --tap                     Print the test output as TAP" << std::endl
      <<  "   -t, --test <test_number>  Run a test specified by its test number" <<std::endl
      <<  "   --csv                     Print the result as CSV (use only together with --test) " << std::endl
      <<  "   --verbose                 Print the expected and the actual result as tables (use only togehter with --test)" << std::endl
      <<  "   -?, --help                Display this help text and exit" <<std::endl;
    return 0;
  }

  if (flags.getArgv().size() > 0) {
    printError(StringUtil::format(
        "invalid argument: '$0', run evql-test-sql --help for help\n",
        flags.getArgv()[0]));

    return 1;
  }

  if (flags.isSet("csv") && !flags.isSet("test")) {
    printError("--csv can only be used if --test is set, run evql-test-sql --help for help\n");
    return 1;
  }

  if (flags.isSet("verbose") && !flags.isSet("test")) {
    printError("--verbose can only be used if --test is set, run evql-test-sql --help for help\n");
    return 1;
  }

  auto output_format = kDefaultOutputFormat;
  if (flags.isSet("tap")) {
    output_format = OutputFormat::TAP;
  }

  if (flags.isSet("csv")) {
    output_format = OutputFormat::CSV;
  }

  if (flags.isSet("verbose")) {
    output_format = OutputFormat::VERBOSE;
  }

  std::string test_nr;
  if (flags.isSet("test")) {
    test_nr = flags.getString("test");
  }

  try {
    auto stdout_os = OutputStream::getStdout();
    bool test_executed = false;

    auto is = FileInputStream::openFile(kTestListFile);

    size_t count = 1;
    std::string line;
    while (is->readLine(&line)) {
      StringUtil::chomp(&line);

      if (!test_nr.empty() && !StringUtil::beginsWith(line, test_nr)) {
        line.clear();
        continue;
      }

      auto ret = runTest(line, output_format);
      if (ret.isSuccess()) {
        stdout_os->write(StringUtil::format(
            "ok $0 - [$1] Test passed\n",
            count,
            line));

      } else {

        switch (output_format) {
          case OutputFormat::TAP:
            stdout_os->write(StringUtil::format(
                "not ok $0 - [$1] ",
                count,
                line));

          case OutputFormat::VERBOSE:
          case OutputFormat::CSV:
            stdout_os->write(StringUtil::format("$0\n", ret.message()));
        }
      }

      line.clear();
      test_executed = true;
      ++count;
    }

    if (!test_executed) {
      printError(StringUtil::format(
          "could not find a test with number $0", test_nr));
      return 1;
    }

  } catch (const std::exception& e) {
    printError(e.what());
    return 1;
  }

  return 0;
}

