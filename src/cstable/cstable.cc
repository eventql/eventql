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
#include "cstable/LEB128ColumnWriter.h"
#include "cstable/StringColumnWriter.h"
#include "cstable/DoubleColumnWriter.h"
#include "cstable/BooleanColumnWriter.h"

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
      ctype = Human::detectDataTypeSeries(row[i], ctype);
    }
  }

  HashMap<String, RefPtr<cstable::ColumnWriter>> column_writers;
  for (const auto& col : column_types) {
    switch (col.second) {

        case HumanDataType::UNSIGNED_INTEGER:
          column_writers.emplace(
              col.first,
              new cstable::LEB128ColumnWriter(0, 0));
          break;

        case HumanDataType::UNSIGNED_INTEGER_NULLABLE:
          column_writers.emplace(
              col.first,
              new cstable::LEB128ColumnWriter(0, 1));
          break;

        case HumanDataType::SIGNED_INTEGER:
          column_writers.emplace(
              col.first,
              new cstable::DoubleColumnWriter(0, 0)); // FIXME
          break;

        case HumanDataType::SIGNED_INTEGER_NULLABLE:
          column_writers.emplace(
              col.first,
              new cstable::DoubleColumnWriter(0, 1)); // FIXME
          break;

        case HumanDataType::FLOAT:
          column_writers.emplace(
              col.first,
              new cstable::DoubleColumnWriter(0, 0));
          break;

        case HumanDataType::FLOAT_NULLABLE:
          column_writers.emplace(
              col.first,
              new cstable::DoubleColumnWriter(0, 1));
          break;

        case HumanDataType::BOOLEAN:
          column_writers.emplace(
              col.first,
              new cstable::BooleanColumnWriter(0, 0));
          break;

        case HumanDataType::BOOLEAN_NULLABLE:
          column_writers.emplace(
              col.first,
              new cstable::BooleanColumnWriter(0, 1));
          break;

        case HumanDataType::DATETIME:
        case HumanDataType::DATETIME_NULLABLE:
        case HumanDataType::URL:
        case HumanDataType::URL_NULLABLE:
        case HumanDataType::CURRENCY:
        case HumanDataType::CURRENCY_NULLABLE:
        case HumanDataType::TEXT:
          column_writers.emplace(
              col.first,
              new cstable::StringColumnWriter(0, 1));
          break;

        case HumanDataType::NULL_OR_EMPTY:
          column_writers.emplace(col.first, nullptr);
          break;

        case HumanDataType::UNKNOWN:
        case HumanDataType::BINARY:
          RAISEF(kTypeError, "invalid column type for column '$0'", col.first);

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

