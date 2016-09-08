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
#include "eventql/eventql.h"
#include "eventql/util/application.h"
#include "eventql/util/logging.h"
#include "eventql/util/cli/CLI.h"
#include "eventql/util/io/inputstream.h"
#include "eventql/util/cli/term.h"
#include "eventql/util/cli/flagparser.h"
#include "eventql/cli/cli_config.h"


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
      "threads",
      cli::FlagParser::T_INTEGER,
      false,
      "t",
      NULL,
      "number of threads",
      "<threads>");

  flags.defineFlag(
      "requests",
      cli::FlagParser::T_INTEGER,
      false,
      "r",
      NULL,
      "number of requests",
      "<requests>");

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

  logInfo("evqlbenchmark", "starting benchmaek");
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
    cfg_builder.setProperty("evqlbenchmark", "host", flags.getString("host"));
  }

  if (flags.isSet("port")) {
    cfg_builder.setProperty(
        "evqlbenchmark",
        "port",
        StringUtil::toString(flags.getInt("port")));
  }

  if (flags.isSet("database")) {
    cfg_builder.setProperty("evql", "database", flags.getString("database"));
  }

  eventql::cli::CLIConfig cfg(cfg_builder.getConfig());

  /* connect to eventql client */
  auto client = evql_client_init();
  if (!client) {
    logFatal("evqlbenchmark", "can't initialize eventql client");
    return 0;
  }

  auto rc = evql_client_connect(
      client,
      cfg.getHost().c_str(),
      cfg.getPort(),
      0);

  logInfo("evqlbenchmark", "connecting to eventql client on $0:$1", cfg.getHost(), cfg.getPort());
  if (rc < 0) {
    logFatal("evqlbenchmark", "can't connect to eventql client: $0", evql_client_geterror(client));
    return 0;
  }
  return 1;
}
