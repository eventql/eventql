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
#include <iostream>
#include <eventql/cli/benchmarks/local_sql.h>
#include <eventql/util/cli/flagparser.h>

namespace eventql {
namespace cli {

const String LocalSQLBenchmark::kName_ = "local-sql";
const String LocalSQLBenchmark::kDescription_ = "Benchmark the SQL engine";

Status LocalSQLBenchmark::execute(
    const std::vector<std::string>& argv,
    FileInputStream* stdin_is,
    OutputStream* stdout_os,
    OutputStream* stderr_os) {

  std::cout << "here be dragons" << std::endl;

  return Status::success();
}

const String& LocalSQLBenchmark::getName() const {
  return kName_;
}

const String& LocalSQLBenchmark::getDescription() const {
  return kDescription_;
}

void LocalSQLBenchmark::printHelp(OutputStream* stdout_os) const {
  std::cout << StringUtil::format(
      "evqlbench-$0 - $1\n\n",
      kName_,
      kDescription_);

  std::cout <<
      "Usage: evqlbench local-sql [OPTIONS]\n\n"
      "   -q, --query <query>        Specify the SQL query to run\n";

}

} // namespace cli
} // namespace eventql


