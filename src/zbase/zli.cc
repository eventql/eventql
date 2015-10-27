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
#include "stx/wallclock.h"
#include "stx/json/json.h"
#include "stx/json/jsonrpc.h"
#include "stx/http/httpclient.h"
#include "stx/cli/CLI.h"
#include "stx/cli/flagparser.h"

using namespace stx;

stx::thread::EventLoop ev;

void cmd_mr_execute(const cli::FlagParser& flags) {
  const auto& argv = flags.getArgv();
  if (argv.size() != 1) {
    RAISE(kUsageError, "usage: $ zli mr-execute <script.js>");
  }

  auto program_source = FileUtil::read(argv[0]);

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
  auto res = http_client.executeRequest(req);

  if (res.statusCode() != 201) {
    RAISEF(kRuntimeError, "execution failed: $0", res.body().toString());
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
  auto mr_execute_cmd = cli.defineCommand("mr-execute");
  mr_execute_cmd->onCall(std::bind(&cmd_mr_execute, std::placeholders::_1));

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

