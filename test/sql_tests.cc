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
#include "eventql/eventql.h"
#include "eventql/util/stringutil.h"
#include "eventql/util/io/fileutil.h"
#include "eventql/util/csv/CSVOutputStream.h"
#include "eventql/sql/runtime/defaultruntime.h"
#include "eventql/sql/CSTableScanProvider.h"
#include "eventql/sql/result_list.h"
#include "eventql/util/csv/CSVInputStream.h"
#include "test_repository.h"

namespace eventql {
namespace test {
namespace sql {

enum class ResultExpectation { TABLE, CHART, ERROR };

const auto kBaseDirectoryPath = "./test";
const auto kDirectoryPath = FileUtil::joinPaths(kBaseDirectoryPath, "sql");
const auto kTestListFile = FileUtil::joinPaths(
    kBaseDirectoryPath,
    "sql_tests.lst");
const auto kSQLPathEnding = ".sql";
const auto kResultPathEnding = ".result.txt";
const auto kActualResultPathEnding = ".actual.result.txt";
const auto kChartColumnName = "__chart";
const auto kDefaultResultExpectation = ResultExpectation::TABLE;

std::string getResultCSV(
    csql::ResultList* result,
    const std::string& row_sep = "\n") {
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
    csql::ResultList* result,
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
          "wrong column name, expected $0 to be $1",
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
    csql::ResultList* result,
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
    csql::ResultList* result,
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

bool runTest(std::string id) {
  auto sql_file_path = FileUtil::joinPaths(
      kDirectoryPath,
      StringUtil::format("$0$1", id, kSQLPathEnding));

  if (!FileUtil::exists(sql_file_path)) {
    RAISE(
        kIOError,
        StringUtil::format("File does not exist: $0", sql_file_path));
  }

  auto result_file_path = FileUtil::joinPaths(
      kDirectoryPath,
      StringUtil::format("$0$1", id, kResultPathEnding));

  if (!FileUtil::exists(result_file_path)) {
    RAISE(
        kIOError,
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
    RAISE(kRuntimeError, "no input table provided");
  }

  StringUtil::replaceAll(&input_table_path, "--");
  StringUtil::ltrim(&input_table_path);
  StringUtil::chomp(&input_table_path);
  input_table_path = FileUtil::joinPaths(
      kBaseDirectoryPath,
      input_table_path);

  if (!FileUtil::exists(input_table_path)) {
    RAISE(
        kRuntimeError,
        StringUtil::format("file does not exist: $0", input_table_path));
  }

  std::string query;
  sql_is->readUntilEOF(&query);

  /* execute query */
  auto runtime = csql::Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  txn->setTableProvider(
      new csql::CSTableScanProvider("testtable", input_table_path));
  csql::ResultList result;

  std::string error_message;
  try {
    auto qplan = runtime->buildQueryPlan(txn.get(), query);
    qplan->execute(0, &result);
  } catch (const std::exception& e) {
    error_message = e.what();

    if (result_expectation != ResultExpectation::ERROR) {
      RAISE(
          kRuntimeError,
          StringUtil::format("unexpected error: $0", error_message));
    }
  }

  if (result.getNumColumns() == 1 &&
      result.getColumns()[0] == kChartColumnName) {
    result_expectation = ResultExpectation::CHART;
  }

  auto rc = Status::success();
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
    return true;
  }

  /* write file with actual result */
  auto acutal_result_file_path = FileUtil::joinPaths(
      kDirectoryPath,
      StringUtil::format("$0$1", id, kActualResultPathEnding));

  const auto& csv_result = getResultCSV(&result);
  FileUtil::write(acutal_result_file_path, csv_result);

  RAISE(kRuntimeError, rc.message());
}

void setup_sql_tests(TestRepository* repo) {
  auto is = FileInputStream::openFile(kTestListFile);
  std::string test_id;
  while (is->readLine(&test_id)) {
    StringUtil::chomp(&test_id);

    auto test_case = eventql::test::TestCase {
      .test_id = StringUtil::format("SQL-$0", test_id.substr(0, 5)),
      .description = test_id.substr(6),
      .fun = std::bind(runTest, test_id),
      .suites =  { TestSuite::WORLD, TestSuite::SMOKE }
    };

    repo->addTestBundle({ test_case });
    test_id.clear();
  }
}

} // namespace sql
} // namespace test
} // namespace eventql


