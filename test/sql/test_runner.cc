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
#include "eventql/util/inspect.h"
#include "eventql/util/status.h"
#include "eventql/util/stringutil.h"
#include "eventql/util/io/fileutil.h"
#include "eventql/util/io/inputstream.h"
#include "eventql/util/csv/CSVInputStream.h"
#include "eventql/sql/runtime/defaultruntime.h"
#include "eventql/sql/CSTableScanProvider.h"
#include "eventql/sql/result_list.h"

#include "eventql/eventql.h"
using namespace csql;

const auto kTestListPath = "./sql/test.lst";
const auto kSQLPathEnding = ".sql";
const auto kResultPathEnding = ".result.txt";

Status runTest(const std::string& test) {
  auto sql_file_path = StringUtil::format("./sql/$0$1", test, kSQLPathEnding); //FIXME make ./sql constant
  if (!FileUtil::exists(sql_file_path)) {
    return Status(
        eIOError,
        StringUtil::format("File does not exist: $0", sql_file_path));
  }

  auto result_file_path = StringUtil::format(
      "./sql/$0$1",
      test,
      kResultPathEnding); //FIXME make ./sql constant
  if (!FileUtil::exists(result_file_path)) {
    return Status(
        eIOError,
        StringUtil::format("File does not exist: $0", result_file_path));
  }

  /* execute query */
  auto query_buf = FileUtil::read(sql_file_path);
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  txn->setTableProvider(
      new CSTableScanProvider(
          "testtable",
          "./sql_testdata/testtbl.cst"));

  auto qplan = runtime->buildQueryPlan(txn.get(), query_buf.toString());
  ResultList result;
  qplan->execute(0, &result);

  /* compare result */
  auto csv_is = CSVInputStream::openFile(result_file_path);
  std::vector<std::string> columns;
  if (!csv_is->readNextRow(&columns)) {
    return Status(eRuntimeError, "CSV needs a header");
  }

  /* compare columns */
  if (result.getNumColumns() != columns.size()) {
    return Status(eRuntimeError, StringUtil::format(
        "wrong number of columns, expected $0 to be $1",
        result.getNumColumns(),
        columns.size()));
  }

  auto returned_columns = result.getColumns();
  for (size_t i = 0; i < returned_columns.size(); ++i) {
    if (returned_columns[i] != columns[i]) {
      return Status(eRuntimeError, StringUtil::format(
          "wrong columns name, expected $0 to be $1",
          returned_columns[i],
          columns[i]));
    }
  }

  /* compare rows */
  auto num_returned_rows = result.getNumRows();
  size_t count = 0;
  std::vector<std::string> row;
  while (csv_is->readNextRow(&row)) {
    if (count >= num_returned_rows) {
      return Status(eRuntimeError, "not enough rows returned");
    }

    auto returned_row = result.getRow(count);
    if (returned_row.size() != row.size()) {
      return Status(eRuntimeError, StringUtil::format(
          "wrong number of values, expected $0 to be $1",
          returned_row.size(),
          row.size()));
    }

    for (size_t i = 0; i < row.size(); ++i) {
      if (row[i] != returned_row[i]) {
        return Status(eRuntimeError, StringUtil::format(
            "wrong value, expected $0 to be $1",
            returned_row[i],
            row[i]));
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

int main(int argc, const char** argv) {
  int rc = 0;

  try {
    auto is = FileInputStream::openFile(kTestListPath);
    std::string line;
    size_t count = 1;
    while (is->readLine(&line)) {
      StringUtil::chomp(&line);
      auto ret = runTest(line);
      if (ret.isSuccess()) {
        std::cout << "ok " << count << std::endl;
      } else {
        std::cout << "not ok " << count << " - " << ret.message() << std::endl;
      }

      line.clear();
      ++count;
    }

  } catch (const std::exception& e) {
    rc = 1;
    std::cout << e.what();
  }

  return rc;
}

