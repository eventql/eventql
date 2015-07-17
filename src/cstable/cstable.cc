/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "fnord/stdtypes.h"
#include "fnord/application.h"
#include "fnord/cli/flagparser.h"
#include "fnord/cli/CLI.h"
#include "fnord/io/file.h"
#include "fnord/inspect.h"

using namespace fnord;

void cmd_from_csv(const cli::FlagParser& flags) {
  fnord::iputs("fro csv...", 1);
}

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

  cli::CLI cli;

  /* command: from-csv */
  auto from_csv_cmd = cli.defineCommand("from-csv");
  from_csv_cmd->onCall(std::bind(&cmd_from_csv, std::placeholders::_1));

  from_csv_cmd->flags().defineFlag(
      "input_file",
      fnord::cli::FlagParser::T_STRING,
      true,
      "i",
      NULL,
      "input file",
      "<filename>");

  from_csv_cmd->flags().defineFlag(
      "output_file",
      fnord::cli::FlagParser::T_STRING,
      true,
      "o",
      NULL,
      "input file",
      "<filename>");

  cli.call(flags.getArgv());
  return 0;
}

