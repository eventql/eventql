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
#include "eventql/sql/runtime/defaultruntime.h"
#include "eventql/sql/CSTableScanProvider.h"

#include "eventql/eventql.h"
using namespace csql;

const auto kTestListPath = "./sql/test.lst";

Status executeQuery(const std::string& query) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  txn->setTableProvider(
      new CSTableScanProvider(
          "testtable",
          "eventql/sql/testdata/testtbl.cst"));

  //ResultList result;
  //auto qplan = runtime->buildQueryPlan(txn.get(), query);
  //qplan->execute(0, &result);



  return Status::success();

}

Status runTest(const std::string& file) {
  auto file_path = StringUtil::format("./sql/$0", file); //FIXME make ./sql constant
  StringUtil::chomp(&file_path);

  if (!FileUtil::exists(file_path)) {
    return Status(
        eIOError,
        StringUtil::format("File does not exist: $0", file_path));
  }

  auto is = FileInputStream::openFile(file_path);
  std::string line;
  while (is->readLine(&line)) {
    auto rc = executeQuery(line);
    if (!rc.isSuccess()) {
      return rc;
    }

    line.clear();
  }

  return Status::success();
}

int main(int argc, const char** argv) {
  int rc = 0;

  try {

    auto is = FileInputStream::openFile(kTestListPath);
    std::string line;
    while (is->readLine(&line)) {

      StringUtil::chomp(&line);

      if (StringUtil::endsWith(line, ".sql")) {
        auto ret = runTest(line);
        if (!ret.isSuccess()) {
          iputs("$0", ret.message()); //FIXME better error printing
          rc = 1;
          return rc;
        }
      }

      line.clear();

    }

  } catch (const std::exception& e) {
    std::cout << e.what();
  }

  return rc;
}

