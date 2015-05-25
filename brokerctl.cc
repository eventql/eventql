/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stdlib.h>
#include <unistd.h>
#include "fnord-base/stdtypes.h"
#include "fnord-base/inspect.h"
#include "fnord-base/application.h"
#include "fnord-base/cli/flagparser.h"
#include "fnord-base/cli/CLI.h"
#include "fnord-base/thread/eventloop.h"

using namespace fnord;

int main(int argc, const char** argv) {
  fnord::Application::init();
  fnord::Application::logToStderr();

  fnord::cli::FlagParser flags;

  flags.defineFlag(
      "loglevel",
      fnord::cli::FlagParser::T_STRING,
      false,
      NULL,
      "INFO",
      "loglevel",
      "<level>");

  flags.parseArgv(argc, argv);

  Logger::get()->setMinimumLogLevel(
      strToLogLevel(flags.getString("loglevel")));

  /* setup cli */
  cli::CLI cli;

  auto monitor_cmd = cli.defineCommand("monitor");
  monitor_cmd->onCall([] (const cli::FlagParser& flags) {
    fnord::iputs("monitor", 1);
  });

  cli.call(flags.getArgv());

  return 0;
}

