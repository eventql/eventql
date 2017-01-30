/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
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
#include <eventql/cli/benchmarks/local_sql.h>
#include <eventql/util/cli/flagparser.h>
#include <eventql/util/wallclock.h>
#include <eventql/sql/CSTableScanProvider.h>

namespace eventql {
namespace cli {

const String LocalSQLBenchmark::kName_ = "local-sql";
const String LocalSQLBenchmark::kDescription_ = "Benchmark the SQL engine";

LocalSQLBenchmark::LocalSQLBenchmark() : verbose_(false) {}

Status LocalSQLBenchmark::execute(
    const std::vector<std::string>& argv,
    FileInputStream* stdin_is,
    OutputStream* stdout_os,
    OutputStream* stderr_os) {
  ::cli::FlagParser flags;

  flags.defineFlag(
      "query",
      ::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "str",
      "<str>");

  flags.defineFlag(
      "input_cstable",
      ::cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "str",
      "<str>");

  flags.defineFlag(
      "verbose",
      ::cli::FlagParser::T_SWITCH,
      false,
      NULL,
      NULL,
      "flag",
      "<flag>");

  flags.defineFlag(
      "num",
      ::cli::FlagParser::T_INTEGER,
      false,
      "n",
      NULL,
      "total number of request (inifite if unset)",
      "<rate>");

  flags.parseArgv(argv);
  query_ = flags.getString("query");
  if (flags.isSet("verbose")) {
    verbose_ = true;
  }

  auto runtime = csql::Runtime::getDefaultRuntime();
  auto tables = mkRef(new csql::TableRepository());

  for (const auto& t : flags.getStrings("input_cstable")) {
    auto parts = StringUtil::split(t, ":");
    if (parts.size() != 2) {
      return ReturnCode::errorf("EARG", "invalid argument: ", t);
    }

    tables->addProvider(new csql::CSTableScanProvider(parts[0], parts[1]));
  }

  size_t num_requests = flags.isSet("num") ? flags.getInt("num") : -1;
  for (size_t i = 0; i < num_requests; i++) {
    auto rc = runQuery(runtime.get(), tables.get());

    if (!rc.isSuccess()) {
      return rc;
    }
  }

  return Status::success();
}


ReturnCode LocalSQLBenchmark::runQuery(
    csql::Runtime* runtime,
    csql::TableProvider* tables) const {
  auto txn = runtime->newTransaction();
  txn->setTableProvider(tables);

  auto t0 = WallClock::unixMicros();

  csql::ResultList result;
  try {
    auto qplan = runtime->buildQueryPlan(txn.get(), query_);
    qplan->execute(0, &result);
  } catch (const std::exception& e) {
    return ReturnCode::error("ERUNTIME", e.what());
  }

  auto t1 = WallClock::unixMicros();

  if (verbose_) {
    result.debugPrint();
  }

  std::cout
      << "took " << (t1-t0) / 1000.0f << "ms" << "; "
      << result.getNumRows() << " rows returned" << std::endl;

  return ReturnCode::success();
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
      "   -q, --query <query>        Specify the SQL query to run\n\n"
      "   -n, --num   <num>           Maximum total number of request (default is infinite)\n";

}

} // namespace cli
} // namespace eventql

