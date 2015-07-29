/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include "stx/stdtypes.h"
#include "stx/application.h"
#include "stx/cli/flagparser.h"
#include "stx/cli/CLI.h"
#include "stx/csv/CSVInputStream.h"
#include "stx/io/file.h"
#include "stx/inspect.h"
#include "stx/human.h"
#include "cstable/LEB128ColumnWriter.h"
#include "cstable/StringColumnWriter.h"
#include "cstable/DoubleColumnWriter.h"
#include "cstable/BooleanColumnWriter.h"
#include "cstable/CSTableWriter.h"

using namespace stx;

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

  csv->rewind();
  csv->skipNextRow();

  size_t num_records = 0;
  while (csv->readNextRow(&row)) {
    ++num_records;

    for (size_t i = 0; i < row.size() && i < columns.size(); ++i) {
      auto& col = column_writers[columns[i]];
      const auto& val = row[i];

      if (col.get() == nullptr) {
        continue;
      }

      if (Human::isNullOrEmpty(val)) {
        col->addNull(0, 0);
        continue;
      }

      switch (col->fieldType()) {

        case msg::FieldType::STRING: {
          col->addDatum(0, col->maxDefinitionLevel(), val.data(), val.size());
          break;
        }

        case msg::FieldType::UINT32: {
          uint32_t v = std::stoull(val);
          col->addDatum(0, col->maxDefinitionLevel(), &v, sizeof(v));
          break;
        }

        case msg::FieldType::UINT64: {
          uint64_t v = std::stoull(val);
          col->addDatum(0, col->maxDefinitionLevel(), &v, sizeof(v));
          break;
        }

        case msg::FieldType::DOUBLE: {
          double v = std::stod(val);
          col->addDatum(0, col->maxDefinitionLevel(), &v, sizeof(v));
          break;
        }

        case msg::FieldType::DATETIME: {
          auto t = Human::parseTime(val);
          auto v = !t.isEmpty() ? UnixTime(0) : t.get();
          col->addDatum(0, col->maxDefinitionLevel(), &v, sizeof(v));
          break;
        }

        case msg::FieldType::BOOLEAN: {
          auto b = Human::parseBoolean(val);
          uint8_t v = !b.isEmpty() && b.get() ? 1 : 0;
          col->addDatum(0, col->maxDefinitionLevel(), &v, sizeof(v));
          break;
        }

        case msg::FieldType::OBJECT:
          RAISE(kIllegalStateError);

      }
    }
  }

  {
    cstable::CSTableWriter writer(flags.getString("output_file"), num_records);
    for (const auto& col : column_writers) {
      if (col.second.get()) {
        writer.addColumn(col.first, col.second.get());
      }
    }

    writer.commit();
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

  /* command: from-csv */
  auto from_csv_cmd = cli.defineCommand("from-csv");
  from_csv_cmd->onCall(std::bind(&cmd_from_csv, std::placeholders::_1));

  from_csv_cmd->flags().defineFlag(
      "input_file",
      stx::cli::FlagParser::T_STRING,
      true,
      "i",
      NULL,
      "input file",
      "<filename>");

  from_csv_cmd->flags().defineFlag(
      "output_file",
      stx::cli::FlagParser::T_STRING,
      true,
      "o",
      NULL,
      "input file",
      "<filename>");

  cli.call(flags.getArgv());
  return 0;
}

