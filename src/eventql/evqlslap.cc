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
#include <iostream>
#include <unistd.h>
#include "eventql/eventql.h"
#include "eventql/util/application.h"
#include "eventql/util/io/inputstream.h"
#include "eventql/util/io/TerminalOutputStream.h"
#include "eventql/util/cli/flagparser.h"
#include "eventql/cli/benchmark.h"

int main(int argc, const char** argv) {
  cli::FlagParser flags;

  flags.defineFlag(
      "host",
      cli::FlagParser::T_STRING,
      false,
      "h",
      "localhost",
      "eventql server hostname",
      "<host>");

  flags.defineFlag(
      "port",
      cli::FlagParser::T_INTEGER,
      false,
      "p",
      "9175",
      "eventql server port",
      "<port>");

  flags.defineFlag(
      "connections",
      cli::FlagParser::T_INTEGER,
      false,
      "c",
      "10",
      "number of connections",
      "<threads>");

  flags.defineFlag(
      "rate",
      cli::FlagParser::T_INTEGER,
      false,
      "r",
      "1",
      "number of requests per second",
      "<rate>");

  flags.defineFlag(
      "num",
      cli::FlagParser::T_INTEGER,
      false,
      "n",
      NULL,
      "total number of request (inifite if unset)",
      "<rate>");

  flags.defineFlag(
      "ignore_errors",
      cli::FlagParser::T_SWITCH,
      false,
      "i",
      NULL,
      "ignore errors",
      "<switch>");

  flags.defineFlag(
      "mode",
      cli::FlagParser::T_STRING,
      false,
      "m",
      "query",
      "mode",
      "<mode>");

  flags.defineFlag(
      "payload",
      cli::FlagParser::T_STRING,
      false,
      "x",
      NULL,
      "payload",
      "<payload>");

  flags.defineFlag(
      "database",
      cli::FlagParser::T_STRING,
      false,
      "d",
      NULL,
      "database",
      "<db>");

  flags.defineFlag(
      "user",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "username",
      "<user>");

  flags.defineFlag(
      "password",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "password",
      "<password>");

  flags.defineFlag(
      "auth_token",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "auth token",
      "<auth_token>");

  flags.defineFlag(
      "help",
      cli::FlagParser::T_SWITCH,
      false,
      "?",
      NULL,
      "help",
      "<help>");

  flags.defineFlag(
      "version",
      cli::FlagParser::T_SWITCH,
      false,
      "v",
      NULL,
      "print version",
      "<switch>");

  Application::init();
  auto stdout_os = TerminalOutputStream::fromStream(OutputStream::getStdout());

  try {
    flags.parseArgv(argc, argv);
  } catch (const std::exception& e) {
    std::cerr << StringUtil::format("ERROR: $0\n", e.what());
    return 1;
  }

  Vector<String> cmd_argv = flags.getArgv();
  bool print_help = flags.isSet("help");
  bool print_version = flags.isSet("version");
  if (print_version || print_help) {
    stdout_os->print(
        StringUtil::format(
            "EventQL $0 ($1)\n"
            "Copyright (c) 2016, DeepCortex GmbH. All rights reserved.\n\n",
            eventql::kVersionString,
            eventql::kBuildID));
  }

  if (print_version) {
    return 1;
  }

  if (print_help) {
    stdout_os->print(
        "Usage: $ evqlslap [OPTIONS]\n\n"
        "   -c, --connections <num>   Number of concurrent connections\n"
        "   -r, --rate <rate>         Maximum rate of requests in RPS\n"
        "   -n, --num <num>           Maximum total number of request (default is infinite)\n"
        "   -i, --ignore_errors       Ignore errors (i.e. continue to run after an error)\n"
        "   -h, --host <hostname>     Set the EventQL server hostname\n"
        "   -p, --port <port>         Set the EventQL server port\n"
        "   -x, --payload <data>      Set the payload (i.e. the sql query)\n"
        "   -d, --database <db>       Select a database\n"
        "   --user <user>             Set the EventQL username\n"
        "   --password <password>     Set the password\n"
        "   --auth_token <token>      Set the EventQL auth token\n"
        "   -?, --help                Display the help text and exit\n"
        "   -v, --version             Display the version of this binary and exit\n");

    return 1;
  }


  /* set up request callback */
  eventql::cli::Benchmark::RequestCallbackType request_handler;
  auto mode = flags.getString("mode");

  if (!request_handler && mode == "query") {
    request_handler = std::bind(
        eventql::cli::benchmark_query,
        std::placeholders::_1,
        flags.getString("database"),
        flags.getString("payload"));
  }

  if (!request_handler) {
    std::cerr << "ERROR: invalid mode: " << mode << std::endl;
    return 1;
  }


  /* set up progress callback */
  auto on_progress = [&stdout_os](eventql::cli::BenchmarkStats* stats) {
    try {
      stdout_os->eraseLine();
      stdout_os->print(StringUtil::format(
          "\rRunning... rate=$0r/s, avg_runtime=$1ms, total=$2, running=$3, "
          "errors=$4 ($5%)",
          stats->getRollingRPS(),
          stats->getRollingAverageRuntime() / double(kMicrosPerMilli),
          stats->getTotalRequestCount(),
          stats->getRunningRequestCount(),
          stats->getTotalErrorCount(),
          stats->getTotalErrorRate() * 100.0f));
    } catch (const std::exception& e) {
      /* fallthrough */
    }
  };


  /* load auth data */
  std::vector<std::pair<std::string, std::string>> auth_data;
  if (flags.isSet("auth_token")) {
    auth_data.emplace_back("auth_token", flags.getString("auth_token"));
  }

  if (flags.isSet("user")) {
    auth_data.emplace_back("user", flags.getString("user"));
  }

  if (flags.isSet("password")) {
    auth_data.emplace_back("password", flags.getString("password"));
  }


  /* run benchmark */
  eventql::cli::Benchmark benchmark(
      flags.getInt("connections"),
      flags.isSet("ignore_errors"),
      flags.getInt("rate"),
      flags.isSet("num") ? flags.getInt("num") : -1);

  benchmark.setRequestHandler(request_handler);
  benchmark.setProgressCallback(on_progress);
  benchmark.setProgressRateLimit(kMicrosPerSecond / 10);

  auto rc = benchmark.connect(
      flags.getString("host"),
      flags.getInt("port"),
      auth_data);

  if (rc.isSuccess()) {
    rc = benchmark.run();
  }


  /* print results */
  stdout_os->eraseLine();

  if (rc.isSuccess()) {
    std::cout << "success" << std::endl;
    return 0;
  } else {
    std::cerr << "ERROR: " << rc.getMessage() << std::endl;
    return 1;
  }

  return 0;
}

