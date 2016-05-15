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

using namespace stx;

stx::thread::EventLoop ev;

String loadAuth(const cli::FlagParser& global_flags) {
  auto auth_file_path = FileUtil::joinPaths(getenv("HOME"), ".z1auth");

  if (FileUtil::exists(auth_file_path)) {
    return FileUtil::read(auth_file_path).toString();
  } else {
    if (global_flags.isSet("auth_token")) {
      return global_flags.getString("auth_token");
    } else {
      RAISE(
          kRuntimeError,
          "no auth token found - you must either call 'zli login' or pass the "
          "--auth_token flag");
    }
  }
}

void runJS(
    const cli::FlagParser& global_flags,
    const String& file) {

  auto stdout_os = OutputStream::getStdout();
  auto stderr_os = TerminalOutputStream::fromStream(OutputStream::getStderr());

  try {
    auto program_source = FileUtil::read(file);

    bool finished = false;
    bool error = false;
    String error_string;
    bool line_dirty = false;
    bool is_tty = stderr_os->isTTY();

    auto event_handler = [&] (const http::HTTPSSEEvent& ev) {
      if (ev.name.isEmpty()) {
        return;
      }

      if (ev.name.get() == "status") {
        auto obj = json::parseJSON(ev.data);
        auto tasks_completed = json::objectGetUInt64(obj, "num_tasks_completed");
        auto tasks_total = json::objectGetUInt64(obj, "num_tasks_total");
        auto tasks_running = json::objectGetUInt64(obj, "num_tasks_running");
        auto progress = json::objectGetFloat(obj, "progress");

        auto status_line = StringUtil::format(
            "[$0/$1] $2 tasks running ($3%)",
            tasks_completed.isEmpty() ? 0 : tasks_completed.get(),
            tasks_total.isEmpty() ? 0 : tasks_total.get(),
            tasks_running.isEmpty() ? 0 : tasks_running.get(),
            progress.isEmpty() ? 0 : progress.get() * 100);

        if (is_tty) {
          stderr_os->eraseLine();
          stderr_os->print("\r" + status_line);
          line_dirty = true;
        } else {
          stderr_os->print(status_line + "\n");
        }

        return;
      }

      if (line_dirty) {
        stderr_os->eraseLine();
        stderr_os->print("\r");
        line_dirty = false;
      }

      if (ev.name.get() == "job_started") {
        //stderr_os->printYellow(">> Job started\n");
        return;
      }

      if (ev.name.get() == "job_finished") {
        finished = true;
        return;
      }

      if (ev.name.get() == "error") {
        error = true;
        error_string = URI::urlDecode(ev.data);
        return;
      }

      if (ev.name.get() == "result") {
        stdout_os->write(URI::urlDecode(ev.data) + "\n");
        return;
      }

      if (ev.name.get() == "log") {
        stderr_os->print(URI::urlDecode(ev.data) + "\n");
        return;
      }

    };

    auto url = StringUtil::format(
        "http://$0/api/v1/mapreduce/execute",
        global_flags.getString("api_host"));

    auto auth_token = loadAuth(global_flags);

    http::HTTPMessage::HeaderList auth_headers;
    auth_headers.emplace_back(
        "Authorization",
        StringUtil::format("Token $0", auth_token));

    if (is_tty) {
      stderr_os->print("Launching job...");
      line_dirty = true;
    } else {
      stderr_os->print("Launching job...\n");
    }

    http::HTTPClient http_client(nullptr);
    auto req = http::HTTPRequest::mkPost(url, program_source, auth_headers);
    auto res = http_client.executeRequest(
        req,
        http::HTTPSSEResponseHandler::getFactory(event_handler));

    if (line_dirty) {
      stderr_os->eraseLine();
      stderr_os->print("\r");
    }

    if (res.statusCode() != 200) {
      error = true;
      error_string = "HTTP Error: " + res.body().toString();
    }

    if (!finished && !error) {
      error = true;
      error_string = "connection to server lost";
    }

    if (error) {
      stderr_os->print(
          "ERROR:",
          { TerminalStyle::RED, TerminalStyle::UNDERSCORE });

      stderr_os->print(" " + error_string + "\n");
      exit(1);
    } else {
      stderr_os->printGreen("Job successfully completed\n");
      exit(0);
    }
  } catch (const StandardException& e) {
    stderr_os->print(
        "ERROR:",
        { TerminalStyle::RED, TerminalStyle::UNDERSCORE });

    stderr_os->print(StringUtil::format(" $0\n", e.what()));
    exit(1);
  }
}

void runSQL(
    const cli::FlagParser& global_flags,
    const String& file) {

  auto stdout_os = OutputStream::getStdout();
  auto stderr_os = TerminalOutputStream::fromStream(OutputStream::getStderr());

  csql::BinaryResultParser res_parser;

  res_parser.onTableHeader([] (const Vector<String>& columns) {
    iputs("columns: $0", columns);
  });

  res_parser.onRow([] (int argc, const csql::SValue* argv) {
    iputs("row: $0", Vector<csql::SValue>(argv, argv + argc));
  });

  res_parser.onError([&stderr_os] (const String& error) {
    stderr_os->print(
        "ERROR:",
        { TerminalStyle::RED, TerminalStyle::UNDERSCORE });

    stderr_os->print(StringUtil::format(" $0\n", error));
    exit(1);
  });

  try {
    auto program_source = FileUtil::read(file).toString();
    bool is_tty = stderr_os->isTTY();

    auto url = StringUtil::format(
        "http://$0/api/v1/sql",
        global_flags.getString("api_host"));

    auto postdata = StringUtil::format(
          "format=binary&query=$0",
          URI::urlEncode(program_source));

    auto auth_token = loadAuth(global_flags);

    http::HTTPMessage::HeaderList auth_headers;
    auth_headers.emplace_back(
        "Authorization",
        StringUtil::format("Token $0", auth_token));

    http::HTTPClient http_client(nullptr);
    auto req = http::HTTPRequest::mkPost(url, postdata, auth_headers);
    auto res = http_client.executeRequest(
        req,
        http::StreamingResponseHandler::getFactory(
            std::bind(
                &csql::BinaryResultParser::parse,
                &res_parser,
                std::placeholders::_1,
                std::placeholders::_2)));

    if (!res_parser.eof()) {
      stderr_os->print(
          "ERROR:",
          { TerminalStyle::RED, TerminalStyle::UNDERSCORE });

      stderr_os->print(" connection to server list");
      exit(1);
    }
  } catch (const StandardException& e) {
    stderr_os->print(
        "ERROR:",
        { TerminalStyle::RED, TerminalStyle::UNDERSCORE });

    stderr_os->print(StringUtil::format(" $0\n", e.what()));
    exit(1);
  }
}

void cmd_run(
    const cli::FlagParser& global_flags,
    const cli::FlagParser& cmd_flags) {
  const auto& argv = cmd_flags.getArgv();
  if (argv.size() != 1) {
    RAISE(kUsageError, "usage: $ zli run <script.js>");
  }

  if (StringUtil::endsWith(argv[0], "js")) {
    runJS(global_flags, argv[0]);
    return;
  }

  if (StringUtil::endsWith(argv[0], "sql")) {
    runSQL(global_flags, argv[0]);
    return;
  }

  RAISE(kUsageError, "unsupported file format");
}

void cmd_login(
    const cli::FlagParser& global_flags,
    const cli::FlagParser& cmd_flags) {
  Term term;

  term.print(">> Username: ");
  auto username = term.readLine();
  term.print(">> Password (will not be echoed): ");
  auto password = term.readPassword();
  term.print("\n");

  try {
    auto url = StringUtil::format(
        "http://$0/api/v1/auth/login",
        global_flags.getString("api_host"));

    auto authdata = StringUtil::format(
        "userid=$0&password=$1",
        URI::urlEncode(username),
        URI::urlEncode(password));

    http::HTTPClient http_client(nullptr);
    auto req = http::HTTPRequest::mkPost(url, authdata);
    auto res = http_client.executeRequest(req);

    switch (res.statusCode()) {

      case 200: {
        auto json = json::parseJSON(res.body());
        auto auth_token = json::objectGetString(json, "auth_token");

        if (!auth_token.isEmpty()) {
          {
            auto auth_file_path = FileUtil::joinPaths(getenv("HOME"), ".z1auth");
            auto file = File::openFile(
              auth_file_path,
              File::O_WRITE | File::O_CREATEOROPEN | File::O_TRUNCATE);

            file.write(auth_token.get());
          }

          term.printGreen("Login successful\n");
          exit(0);
        }

        /* fallthrough */
      }

      case 401: {
        term.printRed("Invalid credentials, please try again\n");
        exit(1);
      }

      default:
        RAISEF(kRuntimeError, "HTTP Error: $0", res.body().toString());

    }
  } catch (const StandardException& e) {
    term.print("ERROR:", { TerminalStyle::RED, TerminalStyle::UNDERSCORE });
    term.print(StringUtil::format(" $0\n", e.what()));
    exit(1);
  }
}

void cmd_version(
    const cli::FlagParser& global_flags,
    const cli::FlagParser& cmd_flags) {
  auto stdout_os = OutputStream::getStdout();
  stdout_os->write("zli v0.0.1\n");
}

int main(int argc, const char** argv) {
  stx::Application::init();
  stx::Application::logToStderr();

  stx::cli::FlagParser flags;

  flags.defineFlag(
      "api_host",
      stx::cli::FlagParser::T_STRING,
      false,
      NULL,
      "api.zscale.io",
      "api host",
      "<host>");

  flags.defineFlag(
      "auth_token",
      stx::cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "auth token",
      "<token>");


  flags.defineFlag(
      "loglevel",
      stx::cli::FlagParser::T_STRING,
      false,
      NULL,
      "INFO",
      "loglevel",
      "<level>");

  flags.parseArgv(argc, argv);

  Logger::get()->setMinimumLogLevel(
      strToLogLevel(flags.getString("loglevel")));

  cli::CLI cli;

  /* command: run */
  auto run_cmd = cli.defineCommand("run");
  run_cmd->onCall(std::bind(&cmd_run, flags, std::placeholders::_1));

  /* command: login */
  auto login_cmd = cli.defineCommand("login");
  login_cmd->onCall(std::bind(&cmd_login, flags, std::placeholders::_1));

  /* command: version */
  auto version_cmd = cli.defineCommand("version");
  version_cmd->onCall(std::bind(&cmd_version, flags, std::placeholders::_1));

  //mr_execute_cmd->flags().defineFlag(
  //    "api_host",
  //    stx::cli::FlagParser::T_STRING,
  //    false,
  //    NULL,
  //    "api.zscale.io",
  //    "api host",
  //    "<host>");

  cli.call(flags.getArgv());
  return 0;
}

