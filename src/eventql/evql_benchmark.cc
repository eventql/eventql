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
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <poll.h>
#include <thread>
#include "eventql/eventql.h"
#include "eventql/util/application.h"
#include "eventql/util/logging.h"
#include "eventql/util/UnixTime.h"
#include "eventql/util/return_code.h"
#include "eventql/util/cli/CLI.h"
#include "eventql/util/io/inputstream.h"
#include "eventql/util/cli/term.h"
#include "eventql/util/cli/flagparser.h"
#include "eventql/util/thread/threadpool.h"
#include "eventql/cli/cli_config.h"


struct TimeWindow {
  static constexpr auto kMillisPerWindow = 1 * kMillisPerSecond;

  TimeWindow() : num_requests_(0) {}

  void addRequest() {
    ++num_requests_;
  }

  int64_t getRemainingMillis() {
    UnixTime now;
    auto duration = now - start_;
    return kMillisPerWindow - duration.milliseconds();
  }

  void clear() {
    num_requests_ = 0;
    UnixTime now;
    start_ = now;
  }

protected:
  size_t num_requests_;
  UnixTime start_;
};

struct RequestStats {
  static constexpr auto kMaxErrors = 10;

  RequestStats() :
      failed_requests_(0),
      successful_requests_(0),
      max_requests_(0),
      has_max_(false) {}

  void addFailedRequest() {
    ++failed_requests_;
  }

  void addSuccessfulRequest() {
    ++successful_requests_;
  }

  uint64_t getFailedRequests() {
    return failed_requests_;
  }

  uint64_t getSuccessfulRequest() {
    return successful_requests_;
  }

  uint64_t getTotal() {
    return successful_requests_ + failed_requests_;
  }

  void setMaximumRequests(uint64_t max_requests) {
    max_requests_ = max_requests;
    has_max_ = true;
  }

  bool stop() {
    if (failed_requests_ >= kMaxErrors)
    if (!has_max_) {
      return false;
    }

    return getTotal() >= max_requests_;
  }

protected:
  uint64_t failed_requests_;
  uint64_t successful_requests_;
  uint64_t max_requests_;
  bool has_max_;
};

ReturnCode sendQuery(
    const String& query,
    const String& qry_db,
    evql_client_t* client) {
  uint64_t qry_flags = 0;
  if (!qry_db.empty()) {
    qry_flags |= EVQL_QUERY_SWITCHDB;
  }

  int rc = evql_query(client, query.c_str(), qry_db.c_str(), qry_flags);

  size_t result_ncols;
  if (rc == 0) {
    rc = evql_num_columns(client, &result_ncols);
  }

  while (rc >= 0) {
    const char** fields;
    size_t* field_lens;
    rc = evql_fetch_row(client, &fields, &field_lens);
    if (rc < 1) {
      break;
    }
  }

  evql_client_releasebuffers(client);

  if (rc == -1) {
    return ReturnCode::error("EIOERROR", evql_client_geterror(client));
  } else {
    return ReturnCode::success();
  }

}

void print(
    size_t num_errors,
    size_t num_succes,
    UnixTime start,
    OutputStream* stdout_os) {
  UnixTime now;
  auto duration = now - start;
  stdout_os->write(StringUtil::format(
    "total   successful    error      milliseconds\n"
    "   $0      $1            $2      $3\n\n",
    num_errors + num_succes,
    num_succes,
    num_errors,
    duration.milliseconds()
  ));
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
      "version",
      cli::FlagParser::T_SWITCH,
      false,
      "v",
      NULL,
      "print version",
      "<switch>");

  flags.defineFlag(
      "host",
      cli::FlagParser::T_STRING,
      false,
      "h",
      NULL,
      "eventql server hostname",
      "<host>");

  flags.defineFlag(
      "port",
      cli::FlagParser::T_INTEGER,
      false,
      "p",
      NULL,
      "eventql server port",
      "<port>");

  flags.defineFlag(
      "database",
      cli::FlagParser::T_STRING,
      false,
      "d",
      NULL,
      "database",
      "<db>");

  flags.defineFlag(
      "query",
      cli::FlagParser::T_STRING,
      true,
      "q",
      NULL,
      "query str",
      "<query>");

  flags.defineFlag(
      "threads",
      cli::FlagParser::T_INTEGER,
      false,
      "t",
      "10",
      "number of threads",
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
      "max",
      cli::FlagParser::T_INTEGER,
      false,
      "n",
      NULL,
      "number of requests per second",
      "<rate>");

  flags.defineFlag(
      "loglevel",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      "INFO",
      "loglevel",
      "<level>");

  flags.parseArgv(argc, argv);
  Vector<String> cmd_argv = flags.getArgv();
  Logger::get()->setMinimumLogLevel(
      strToLogLevel(flags.getString("loglevel")));

  Application::init();
  Application::logToStderr("evqlbenchmark");
  auto stdin_is = InputStream::getStdin();
  auto stdout_os = OutputStream::getStdout();
  auto stderr_os = OutputStream::getStderr();

  bool print_help = flags.isSet("help");
  bool print_version = flags.isSet("version");
  if (print_version || print_help) {
    auto stdout_os = OutputStream::getStdout();
    stdout_os->write(
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
    stdout_os->write(
      "Usage: $ evqlbenchmark [OPTIONS] <command> [<args>]\n\n"
      "   -D, --database <db>       Select a database\n"
      "   -h, --host <hostname>     Set the EventQL server hostname\n"
      "   -p, --port <port>         Set the EventQL server port\n"
      "   -?, --help <topic>        Display a command's help text and exit\n"
      "   -v, --version             Display the version of this binary and exit\n\n"
      "evqlbenchmark commands:\n"
      );
    return 1;
  }

  logInfo("evqlbenchmark", "starting benchmark");
  /* options */
  eventql::ProcessConfigBuilder cfg_builder;
  {
    auto status = cfg_builder.loadDefaultConfigFile("evql");
    if (!status.isSuccess()) {
      logFatal("evqlbenchmark", "error while loading config file %s", status.message());
      return 0;
    }
  }

  if (flags.isSet("host")) {
    cfg_builder.setProperty("evql", "host", flags.getString("host"));
  }

  if (flags.isSet("port")) {
    cfg_builder.setProperty(
        "evql",
        "port",
        StringUtil::toString(flags.getInt("port")));
  }

  if (flags.isSet("database")) {
    cfg_builder.setProperty("evql", "database", flags.getString("database"));
  }

  eventql::cli::CLIConfig cfg(cfg_builder.getConfig());


  String qry_db;
  if (flags.isSet("database")) {
    qry_db = flags.getString("database");
  }

  auto qry_str = flags.getString("query");
  auto rate = flags.getInt("rate");

  const UnixTime global_start;

  std::mutex m;
  TimeWindow twindow;

  RequestStats rstats;
  if (flags.isSet("max")) {
    rstats.setMaximumRequests(flags.getInt("max"));
  }

  auto num_threads = flags.getInt("threads");
  Vector<std::thread> threads;
  for (size_t i = 0; i < num_threads; ++i) {
    threads.emplace_back(std::thread([&] () {

      /* connect to eventql client */
      auto client = evql_client_init();
      if (!client) {
        logFatal("evqlbenchmark", "can't initialize eventql client");
        return 0;
      }

      {
        auto rc = evql_client_connect(
            client,
            cfg.getHost().c_str(),
            cfg.getPort(),
            0);

        if (rc < 0) {
          logFatal("evqlbenchmark", "can't connect to eventql client: $0", evql_client_geterror(client));
          return 0;
        }
      }

      for (;;) {
        m.lock();
        /* check remaining time in current timewindow */
        if (twindow.getRemainingMillis() <= 0) {
          /* start a new timewindow */
          twindow.clear();
          m.unlock();
          continue;
        }
        m.unlock();

        /* send query */
        auto rc = sendQuery(qry_str, qry_db, client);
        m.lock();
        if (!rc.isSuccess()) {
          logFatal("evqlbenchmark", "executing query failed: $0", rc.getMessage());
          rstats.addFailedRequest();
        } else {
          rstats.addSuccessfulRequest();
        }
        m.unlock();

        //print(errors, requests_sent, global_start, stdout_os.get());
        if (rstats.stop()) {
          break;
        }
      }

      evql_client_close(client);
    }));
  }

  for (auto& t : threads) {
    t.join();
  }

  //print(errors, requests_sent, global_start, stdout_os.get());
  return 0;
}
