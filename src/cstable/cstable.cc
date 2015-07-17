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
#include "fnord/csv/CSVInputStream.h"
#include "fnord/io/file.h"
#include "fnord/inspect.h"
#include "fnord/human.h"

using namespace fnord;

void cmd_from_csv(const cli::FlagParser& flags) {
  auto csv = CSVInputStream::openFile(
      flags.getString("input_file"),
      '\t',
      '\n');

  Vector<String> columns;
  csv->readNextRow(&columns);

  HashMap<String, HumanDataType> column_types;
  for (const auto& hdr : columns) {
    column_types[hdr] = HumanDataType::UNKNOWN;
  }

  Vector<String> row;
  while (csv->readNextRow(&row)) {
    for (size_t i = 0; i < row.size() && i < columns.size(); ++i) {
      auto& ctype = column_types[columns[i]];
      ctype = Human::detectDataType(row[i], ctype);
    }
  }

  //fnord::iputs("row... $0", row.size());
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

