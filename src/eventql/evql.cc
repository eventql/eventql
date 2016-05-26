/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
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

#include "eventql/eventql.h"

thread::EventLoop ev;

//String loadAuth(const cli::FlagParser& global_flags) {
//  auto auth_file_path = FileUtil::joinPaths(getenv("HOME"), ".z1auth");
//
//  if (FileUtil::exists(auth_file_path)) {
//    return FileUtil::read(auth_file_path).toString();
//  } else {
//    if (global_flags.isSet("auth_token")) {
//      return global_flags.getString("auth_token");
//    } else {
//      RAISE(
//          kRuntimeError,
//          "no auth token found - you must either call 'zli login' or pass the "
//          "--auth_token flag");
//    }
//  }
//}
//
//
//void runSQL(
//    const cli::FlagParser& global_flags,
//    const String& file) {
//
//  auto stdout_os = OutputStream::getStdout();
//  auto stderr_os = TerminalOutputStream::fromStream(OutputStream::getStderr());
//
//  csql::BinaryResultParser res_parser;
//
//  res_parser.onTableHeader([] (const Vector<String>& columns) {
//    iputs("columns: $0", columns);
//  });
//
//  res_parser.onRow([] (int argc, const csql::SValue* argv) {
//    iputs("row: $0", Vector<csql::SValue>(argv, argv + argc));
//  });
//
//  res_parser.onError([&stderr_os] (const String& error) {
//    stderr_os->print(
//        "ERROR:",
//        { TerminalStyle::RED, TerminalStyle::UNDERSCORE });
//
//    stderr_os->print(StringUtil::format(" $0\n", error));
//    exit(1);
//  });
//
//  try {
//    auto program_source = FileUtil::read(file).toString();
//    bool is_tty = stderr_os->isTTY();
//
//    auto url = StringUtil::format(
//        "http://$0/api/v1/sql",
//        global_flags.getString("api_host"));
//
//    auto postdata = StringUtil::format(
//          "format=binary&query=$0",
//          URI::urlEncode(program_source));
//
//    auto auth_token = loadAuth(global_flags);
//
//    http::HTTPMessage::HeaderList auth_headers;
//    auth_headers.emplace_back(
//        "Authorization",
//        StringUtil::format("Token $0", auth_token));
//
//    http::HTTPClient http_client(nullptr);
//    auto req = http::HTTPRequest::mkPost(url, postdata, auth_headers);
//    auto res = http_client.executeRequest(
//        req,
//        http::StreamingResponseHandler::getFactory(
//            std::bind(
//                &csql::BinaryResultParser::parse,
//                &res_parser,
//                std::placeholders::_1,
//                std::placeholders::_2)));
//
//    if (!res_parser.eof()) {
//      stderr_os->print(
//          "ERROR:",
//          { TerminalStyle::RED, TerminalStyle::UNDERSCORE });
//
//      stderr_os->print(" connection to server list");
//      exit(1);
//    }
//  } catch (const StandardException& e) {
//    stderr_os->print(
//        "ERROR:",
//        { TerminalStyle::RED, TerminalStyle::UNDERSCORE });
//
//    stderr_os->print(StringUtil::format(" $0\n", e.what()));
//    exit(1);
//  }
//}
//
//void cmd_run(
//    const cli::FlagParser& global_flags,
//    const cli::FlagParser& cmd_flags) {
//  const auto& argv = cmd_flags.getArgv();
//  if (argv.size() != 1) {
//    RAISE(kUsageError, "usage: $ zli run <script.js>");
//  }
//
//  if (StringUtil::endsWith(argv[0], "js")) {
//    runJS(global_flags, argv[0]);
//    return;
//  }
//
//  if (StringUtil::endsWith(argv[0], "sql")) {
//    runSQL(global_flags, argv[0]);
//    return;
//  }
//
//  RAISE(kUsageError, "unsupported file format");
//}
//
//void cmd_login(
//    const cli::FlagParser& global_flags,
//    const cli::FlagParser& cmd_flags) {
//  Term term;
//
//  term.print(">> Username: ");
//  auto username = term.readLine();
//  term.print(">> Password (will not be echoed): ");
//  auto password = term.readPassword();
//  term.print("\n");
//
//  try {
//    auto url = StringUtil::format(
//        "http://$0/api/v1/auth/login",
//        global_flags.getString("api_host"));
//
//    auto authdata = StringUtil::format(
//        "userid=$0&password=$1",
//        URI::urlEncode(username),
//        URI::urlEncode(password));
//
//    http::HTTPClient http_client(nullptr);
//    auto req = http::HTTPRequest::mkPost(url, authdata);
//    auto res = http_client.executeRequest(req);
//
//    switch (res.statusCode()) {
//
//      case 200: {
//        auto json = json::parseJSON(res.body());
//        auto auth_token = json::objectGetString(json, "auth_token");
//
//        if (!auth_token.isEmpty()) {
//          {
//            auto auth_file_path = FileUtil::joinPaths(getenv("HOME"), ".z1auth");
//            auto file = File::openFile(
//              auth_file_path,
//              File::O_WRITE | File::O_CREATEOROPEN | File::O_TRUNCATE);
//
//            file.write(auth_token.get());
//          }
//
//          term.printGreen("Login successful\n");
//          exit(0);
//        }
//
//        /* fallthrough */
//      }
//
//      case 401: {
//        term.printRed("Invalid credentials, please try again\n");
//        exit(1);
//      }
//
//      default:
//        RAISEF(kRuntimeError, "HTTP Error: $0", res.body().toString());
//
//    }
//  } catch (const StandardException& e) {
//    term.print("ERROR:", { TerminalStyle::RED, TerminalStyle::UNDERSCORE });
//    term.print(StringUtil::format(" $0\n", e.what()));
//    exit(1);
//  }
//}

void cmd_version(
    const cli::FlagParser& global_flags,
    const cli::FlagParser& cmd_flags) {
  auto stdout_os = OutputStream::getStdout();
  stdout_os->write("zli v0.0.1\n");
}

static bool hasSTDIN() {
  struct pollfd p = {
    .fd = STDIN_FILENO,
    .events = POLLIN | POLLRDBAND | POLLRDNORM | POLLPRI
  };

  return poll(&p, 1, 0) == 1;
}

int main(int argc, const char** argv) {
  Application::init();
  Application::logToStderr();

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
      "sql",
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
      "auth_token",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "auth token",
      "<token>");

  flags.defineFlag(
      "loglevel",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      "INFO",
      "loglevel",
      "<level>");

  flags.parseArgv(argc, argv);

  Logger::get()->setMinimumLogLevel(
      strToLogLevel(flags.getString("loglevel")));

  auto stdin_is = InputStream::getStdin();
  auto stdout_os = OutputStream::getStdout();
  auto stderr_os = OutputStream::getStderr();

  /* print help */
  if (flags.isSet("help")) {
    stdout_os->write(
        "EventQL v0.3.0 - <build info>\n"
        "Copyright (c) 2016, zScale Techology GmbH. All rights reserved.\n\n"
    );

    stdout_os->write(
        "Usage: $ evql [OPTIONS] [query]\n"
        "       $ evql [OPTIONS] -f file\n"
        "  -?, --help              Display this help text and exit\n"
        "  -f, --file <file>       Read query from file\n"
        "  -e, --exec <query_str>  Execute query string\n"
        "  -l, --lang <lang>       Set the query language ('sql' or 'js')\n"
        "  -h, --host <hostname>   Set the EventQL server hostname\n"
        "  -p, --port <port>       Set the EventQL server port\n"
        "  -u, --user <user>       Set the auth username\n"
        "  --password <password>   Set the auth password (if required)\n"
        "  --auth_token <token>    Set the auth token (if required)\n"
        "  -B, --batch             Run in batch mode (streaming result output)\n"
        "  -q, --quiet             Be quiet (disables query progress)\n"
        "  -v, --verbose           Print debug output to STDERR\n"
        "  --version               Display the version of this binary and exit\n"
        "                                                       \n"
        "Examples:                                              \n"
        "  $ evql                        # start an interactive shell\n"
        "  $ evql -h localhost -p 9175   # start an interactive shell\n"
        "  $ evql < query.sql            # execute query from STDIN\n"
        "  $ evql -f query.sql           # execute query in query.sql\n"
        "  $ evql -l js -f query.js      # execute query in query.js\n"
        "  $ evql -e 'SELECT 42;'        # execute 'SELECT 42'\n"
    );
    return 0;
  }

  /* console options */
  eventql::cli::ConsoleOptions console_opts;
  console_opts.server_host = flags.getString("host");
  console_opts.server_port = flags.getInt("port");
  console_opts.server_auth_token = flags.getString("auth_token");

  /* cli */
  eventql::cli::Console console(console_opts);

  if (flags.getArgv().size() > 0) {
    stderr_os->write(
        StringUtil::format(
            "invalid argument: '$0', run evql --help for help\n",
            flags.getArgv()[0]));

    return 1;
  }

  if (flags.isSet("file")) {
    auto lang = flags.getString("lang");
    auto query = FileUtil::read(flags.getString("file"));

    if (lang == "js" || lang == "javascript") {
      auto ret = console.runJS(query.toString());
      return ret.isSuccess() ? 0 : 1;
    } else {
      auto ret = console.runQuery(query.toString());
      return ret.isSuccess() ? 0 : 1;
    }
  }

  if (flags.isSet("exec")) {
    auto ret = console.runQuery(flags.getString("exec"));
    return ret.isSuccess() ? 0 : 1;
  }

  if (hasSTDIN() || flags.isSet("batch") || !stdout_os->isTTY()) {
    String query;
    while (stdin_is->readLine(&query)) {
      auto ret = console.runQuery(query);
      if (ret.isError()) {
        return 1;
      }
    }

    return 0;
  }

  console.startInteractiveShell();
  return 0;
}

