/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "stx/application.h"
#include "stx/logging.h"
#include "stx/random.h"
#include "stx/thread/eventloop.h"
#include "stx/thread/threadpool.h"
#include "stx/thread/FixedSizeThreadPool.h"
#include "stx/io/TerminalOutputStream.h"
#include "stx/wallclock.h"
#include "stx/json/json.h"
#include "stx/json/jsonrpc.h"
#include "stx/http/httpclient.h"
#include "stx/http/HTTPSSEResponseHandler.h"
#include "stx/cli/CLI.h"
#include "stx/cli/flagparser.h"

using namespace stx;

stx::thread::EventLoop ev;

void cmd_run(const cli::FlagParser& flags) {
  const auto& argv = flags.getArgv();
  if (argv.size() != 1) {
    RAISE(kUsageError, "usage: $ zli run <script.js>");
  }

  auto stdout_os = OutputStream::getStdout();
  auto stderr_os = OutputStream::getStderr();
  auto program_source = FileUtil::read(argv[0]);

  stderr_os->write(">> Launching job...\n");

  bool finished = false;
  bool error = false;
  String error_string;
  bool line_dirty = false;

  auto event_handler = [&] (const http::HTTPSSEEvent& ev) {
    if (ev.name.isEmpty()) {
      return;
    }

    if (ev.name.get() == "status") {
      auto obj = json::parseJSON(ev.data);
      auto tasks_completed = json::objectGetUInt64(obj, "num_tasks_completed");
      auto tasks_total = json::objectGetUInt64(obj, "num_tasks_total");
      auto tasks_running = json::objectGetUInt64(obj, "num_tasks_running");

      stderr_os->write(
          StringUtil::format(
              "\r[$0/$1] $2 tasks running",
              tasks_completed.isEmpty() ? 0 : tasks_completed.get(),
              tasks_total.isEmpty() ? 0 : tasks_total.get(),
              tasks_running.isEmpty() ? 0 : tasks_running.get()));

      line_dirty = true;
      return;
    }

    if (line_dirty) {
      stderr_os->write("\r                                                \r");
      line_dirty = false;
    }

    if (ev.name.get() == "job_started") {
      stderr_os->write(">> Job started\n");
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
      stderr_os->write(URI::urlDecode(ev.data) + "\n");
      return;
    }

  };

  auto url = StringUtil::format(
      "http://$0/api/v1/mapreduce/execute",
      flags.getString("api_host"));

  auto auth_token = flags.getString("auth_token");

  http::HTTPMessage::HeaderList auth_headers;
  auth_headers.emplace_back(
      "Authorization",
      StringUtil::format("Token $0", auth_token));

  http::HTTPClient http_client;
  auto req = http::HTTPRequest::mkPost(url, program_source, auth_headers);
  auto res = http_client.executeRequest(
      req,
      http::HTTPSSEResponseHandler::getFactory(event_handler));

  if (res.statusCode() != 200) {
    error = true;
    error_string = "HTTP Error: " + res.body().toString();
  }

  if (!finished && !error) {
    error = true;
    error_string = "connection to server lost";
  }

  if (error) {
    stderr_os->write(StringUtil::format("ERROR: $0\n", error_string));
    exit(1);
  } else {
    stderr_os->write(">> Job successfully completed\n");
    exit(0);
  }
}

int main(int argc, const char** argv) {
  stx::Application::init();
  stx::Application::logToStderr();

  stx::cli::FlagParser flags;

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

  /* command: mr_execute */
  auto mr_execute_cmd = cli.defineCommand("run");
  mr_execute_cmd->onCall(std::bind(&cmd_run, std::placeholders::_1));

  mr_execute_cmd->flags().defineFlag(
      "api_host",
      stx::cli::FlagParser::T_STRING,
      false,
      NULL,
      "api.zscale.io",
      "api host",
      "<host>");

  mr_execute_cmd->flags().defineFlag(
      "auth_token",
      stx::cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "auth token",
      "<token>");

  cli.call(flags.getArgv());
  return 0;
}

