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
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <poll.h>
#include <thread>

#include "eventql/eventql.h"
#include "eventql/util/application.h"
#include "eventql/util/logging.h"
#include "eventql/util/io/fileutil.h"
#include "eventql/util/io/TerminalOutputStream.h"
#include "eventql/util/wallclock.h"
#include "eventql/util/cli/CLI.h"
#include "eventql/util/cli/flagparser.h"
#include "eventql/util/cli/term.h"
#include "eventql/cli/console.h"
#include "eventql/cli/cli_config.h"

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
      "config",
      ::cli::FlagParser::T_STRING,
      false,
      "c",
      NULL,
      "path to config file",
      "<config_file>");

  flags.defineFlag(
      "config_set",
      ::cli::FlagParser::T_STRING,
      false,
      "C",
      NULL,
      "set config option",
      "<key>=<val>");

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
      "output_file",
      cli::FlagParser::T_STRING,
      false,
      "o",
      NULL,
      "output file",
      "<path>");

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
        "   -c, --config <file>       Load config from file\n"
        "   -C name=value             Define a config value on the command line\n"
        "   -o --output_file <file>   Save the output to a file\n"
        "   -B, --batch               Run in batch mode (streaming result output)\n"
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
  cfg_builder.setClientDefaults();

  if (flags.isSet("config")) {
    auto config_file_path = flags.getString("config");
    auto rc = cfg_builder.loadFile(config_file_path);
    if (!rc.isSuccess()) {
      printError("error while loading config file: " + rc.message());
      return 1;
    }
  }

  for (const auto& opt : flags.getStrings("config_set")) {
    auto opt_key_end = opt.find("=");
    if (opt_key_end == String::npos) {
      printError(StringUtil::format("invalid config option: $0", opt));;
      return 1;
    }

    auto opt_key = opt.substr(0, opt_key_end);
    auto opt_value = opt.substr(opt_key_end + 1);
    cfg_builder.setProperty(opt_key, opt_value);
  }

  {
    auto status = cfg_builder.loadDefaultConfigFile("evql");
    if (!status.isSuccess()) {
      printError(status.message());
      return 1;
    }
  }

  if (flags.isSet("host")) {
    cfg_builder.setProperty("client.host", flags.getString("host"));
  }

  if (flags.isSet("port")) {
    cfg_builder.setProperty(
        "client.port",
        StringUtil::toString(flags.getInt("port")));
  }

  if (flags.isSet("database")) {
    cfg_builder.setProperty("client.database", flags.getString("database"));
  }

  if (flags.isSet("user")) {
    cfg_builder.setProperty("client.user", flags.getString("user"));
  }

  if (flags.isSet("password")) {
    cfg_builder.setProperty("client.password", flags.getString("password"));
  }

  if (flags.isSet("auth_token")) {
    cfg_builder.setProperty("client.auth_token", flags.getString("auth_token"));
  }

  if (flags.isSet("file")) {
    cfg_builder.setProperty("client.file", flags.getString("file"));
  }

  if (flags.isSet("lang")) {
    cfg_builder.setProperty("client.lang", flags.getString("lang"));
  }

  if (flags.isSet("output_file")) {
    cfg_builder.setProperty(
        "client.output_file",
        flags.getString("output_file"));
    cfg_builder.setProperty("client.batch", "true");
  }

  if (flags.isSet("batch")) {
    cfg_builder.setProperty("client.batch", "true");
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
      printError(StringUtil::format(
          "error while connecting to server: $0", rc.getMessage()));
      return 1;
    }
  }

  int rc = 0;

  auto file = cli_cfg.getFile();
  auto language = cli_cfg.getLanguage();
  if (file.isEmpty() &&
      !language.isEmpty() &&
      language.get() == eventql::cli::CLIConfig::kLanguage::JAVASCRIPT) {
    printError("missing --file flag. Set --file for javascript");
    rc = 1;
    goto exit;
  }

  if (!file.isEmpty()) {
    if (language.isEmpty()) {
      printError(
          "invalid --lang flag. Must be one of 'sql', 'js' or 'javascript'");
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

  if (hasSTDIN() || cli_cfg.getBatchMode() || !stdout_os->isTTY()) {
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

