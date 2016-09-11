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
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <poll.h>

#include "eventql/eventql.h"
#include "eventql/util/application.h"
#include "eventql/util/logging.h"
#include "eventql/util/random.h"
#include "eventql/util/thread/eventloop.h"
#include "eventql/util/thread/threadpool.h"
#include "eventql/util/thread/FixedSizeThreadPool.h"
#include "eventql/util/io/TerminalOutputStream.h"
#include "eventql/util/wallclock.h"
#include "eventql/util/json/json.h"
#include "eventql/util/json/jsonrpc.h"
#include "eventql/util/http/httpclient.h"
#include "eventql/util/http/HTTPSSEResponseHandler.h"
#include "eventql/util/cli/CLI.h"
#include "eventql/util/cli/flagparser.h"
#include "eventql/util/cli/term.h"
#include "eventql/server/sql/codec/binary_codec.h"
#include "eventql/cli/console.h"
#include "eventql/cli/cli_config.h"

thread::EventLoop ev;

static bool hasSTDIN() {
  struct pollfd p = {
    .fd = STDIN_FILENO,
    .events = POLLIN | POLLRDBAND | POLLRDNORM | POLLPRI
  };

  return poll(&p, 1, 0) == 1;
}

static void printError(const String& error_string) {
  auto stderr_os = TerminalOutputStream::fromStream(OutputStream::getStderr());
  stderr_os->print(
      "ERROR:",
      { TerminalStyle::RED, TerminalStyle::UNDERSCORE });
  stderr_os->print(" " + error_string + "\n");
}

int main(int argc, const char** argv) {
  Application::init();
  Application::logToStderr("evql");

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
      "file",
      cli::FlagParser::T_STRING,
      false,
      "f",
      NULL,
      "query file",
      "<file>");

  flags.defineFlag(
      "lang",
      cli::FlagParser::T_STRING,
      false,
      "l",
      NULL,
      "query language",
      "<lang>");

  flags.defineFlag(
      "exec",
      cli::FlagParser::T_STRING,
      false,
      "e",
      NULL,
      "query string",
      "<query_str>");

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
      "user",
      cli::FlagParser::T_STRING,
      false,
      "u",
      NULL,
      "username",
      "<user>");

  flags.defineFlag(
      "database",
      cli::FlagParser::T_STRING,
      false,
      "d",
      NULL,
      "database",
      "<db>");

  flags.defineFlag(
      "auth_token",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "auth token",
      "<token>");

  flags.defineFlag(
      "history_file",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "history path",
      "<path>");

  flags.defineFlag(
      "history_maxlen",
      cli::FlagParser::T_INTEGER,
      false,
      NULL,
      NULL,
      "history length",
      "<number of history entries>");

  flags.defineFlag(
      "loglevel",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      "INFO",
      "loglevel",
      "<level>");

  flags.defineFlag(
      "batch",
      cli::FlagParser::T_SWITCH,
      false,
      "B",
      NULL,
      "batch",
      "<batch mode>");

  flags.defineFlag(
      "quiet",
      cli::FlagParser::T_SWITCH,
      false,
      "q",
      NULL,
      "quiet",
      "<quiet>");

  flags.parseArgv(argc, argv);

  Logger::get()->setMinimumLogLevel(
      strToLogLevel(flags.getString("loglevel")));

  auto stdin_is = InputStream::getStdin();
  auto stdout_os = OutputStream::getStdout();
  auto stderr_os = OutputStream::getStderr();

  /* print help */
  if (flags.isSet("help") || flags.isSet("version")) {
    stdout_os->write(
        StringUtil::format(
            "EventQL $0 ($1)\n"
            "Copyright (c) 2016, DeepCortex GmbH. All rights reserved.\n\n",
            eventql::kVersionString,
            eventql::kBuildID));
  }

  if (flags.isSet("version")) {
    return 0;
  }

  /* print help */
  if (flags.isSet("help")) {
    stdout_os->write(
        "Usage: $ evql [OPTIONS] [query]\n"
        "       $ evql [OPTIONS] -f file\n\n"
        "   -f, --file <file>         Read query from file\n"
        "   -e, --exec <query_str>    Execute query string\n"
        "   -l, --lang <lang>         Set the query language ('sql' or 'js')\n"
        "   -D, --database <db>       Select a database\n"
        "   -h, --host <hostname>     Set the EventQL server hostname\n"
        "   -p, --port <port>         Set the EventQL server port\n"
        "   -u, --user <user>         Set the auth username\n"
        "   --password <password>     Set the auth password (if required)\n"
        "   --auth_token <token>      Set the auth token (if required)\n"
        "   -B, --batch               Run in batch mode (streaming result output)\n"
        "   --history_file <path>     Set the history file path\n"
        "   --history_max_len <len>   Set the maximum length of the history\n"
        "   -q, --quiet               Be quiet (disables query progress)\n"
        "   --verbose                 Print debug output to STDERR\n"
        "   -v, --version             Display the version of this binary and exit\n"
        "   -?, --help                Display this help text and exit\n"
        "                                                       \n"
        "Examples:                                              \n"
        "   $ evql                        # start an interactive shell\n"
        "   $ evql -h localhost -p 9175   # start an interactive shell\n"
        "   $ evql < query.sql            # execute query from STDIN\n"
        "   $ evql -f query.sql           # execute query in query.sql\n"
        "   $ evql -l js -f query.js      # execute query in query.js\n"
        "   $ evql -e 'SELECT 42;'        # execute 'SELECT 42'\n"
    );
    return 0;
  }

  /* console options */
  eventql::ProcessConfigBuilder cfg_builder;
  {
    auto status = cfg_builder.loadDefaultConfigFile("evql");
    if (!status.isSuccess()) {
      printError(status.message());
      return 1;
    }
  }

  if (flags.isSet("host")) {
    cfg_builder.setProperty("evql", "host", flags.getString("host"));
  }

  if (flags.isSet("auth_token")) {
    cfg_builder.setProperty("evql", "auth_token", flags.getString("auth_token"));
  }

  if (flags.isSet("port")) {
    cfg_builder.setProperty("evql", "port", StringUtil::toString(flags.getInt("port")));
  }

  if (flags.isSet("user")) {
    cfg_builder.setProperty("evql", "user", flags.getString("user"));
  }

  if (flags.isSet("database")) {
    cfg_builder.setProperty("evql", "database", flags.getString("database"));
  }

  if (flags.isSet("file")) {
    cfg_builder.setProperty("evql", "file", flags.getString("file"));
  }

  if (flags.isSet("lang")) {
    cfg_builder.setProperty("evql", "lang", flags.getString("lang"));
  }

  if (flags.isSet("batch")) {
    cfg_builder.setProperty("evql", "batch", "true");
  }

  if (flags.isSet("quiet")) {
    cfg_builder.setProperty("evql", "quiet", "true");
  }

  if (flags.isSet("history_file")) {
    cfg_builder.setProperty(
        "evql",
        "history_file",
        flags.getString("history_file"));
  }

  if (flags.isSet("history_maxlen")) {
    cfg_builder.setProperty(
        "evql",
        "history_maxlen",
        StringUtil::toString(flags.getInt("history_maxlen")));
  }

  /* cli config */
  eventql::cli::CLIConfig cli_cfg(cfg_builder.getConfig());

  if (flags.getArgv().size() > 0) {
    printError(StringUtil::format(
        "invalid argument: '$0', run evql --help for help\n",
        flags.getArgv()[0]));

    return 1;
  }

  /* cli */
  eventql::cli::Console console(cli_cfg);

  {
    auto rc = console.connect();
    if (!rc.isSuccess()) {
      logFatal("evql", "error while connecting to server: $0", rc.getMessage());
      return 1;
    }
  }

  int rc = 0;

  auto file = cli_cfg.getFile();
  auto language = cli_cfg.getLanguage();
  if (file.isEmpty() &&
      !language.isEmpty() &&
      language.get() == eventql::cli::CLIConfig::kLanguage::JAVASCRIPT) {
    logFatal("evql", "missing --file flag. Set --file for javascript");
    rc = 1;
    goto exit;
  }

  if (!file.isEmpty()) {
    if (language.isEmpty()) {
      logFatal("evql", "invalid --lang flag. Must be one of 'sql', 'js' or 'javascript'");
      rc = 1;
      goto exit;
    }

    auto query = FileUtil::read(file.get());
    switch (language.get()) {
      case eventql::cli::CLIConfig::kLanguage::SQL: {
        Status ret = console.runQuery(query.toString());
        rc = ret.isSuccess() ? 0 : 1;
        goto exit;
      }

      case eventql::cli::CLIConfig::kLanguage::JAVASCRIPT: {
        Status ret = console.runJS(query.toString());
        rc = ret.isSuccess() ? 0 : 1;
        goto exit;
      }
    }
  }

  if (flags.isSet("exec")) {
    auto ret = console.runQuery(flags.getString("exec"));
    rc = ret.isSuccess() ? 0 : 1;
    goto exit;
  }

  if (hasSTDIN() || flags.isSet("batch") || !stdout_os->isTTY()) {
    String query;
    while (stdin_is->readLine(&query)) {
      auto ret = console.runQuery(query);
      query = "";
      if (ret.isError()) {
        rc = 1;
        goto exit;
      }
    }
  } else {
    console.startInteractiveShell();
  }

exit:
  console.close();
  return rc;
}

