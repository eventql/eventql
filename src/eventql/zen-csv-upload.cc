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
#include "eventql/util/stdtypes.h"
#include "eventql/util/application.h"
#include "eventql/util/cli/flagparser.h"
#include "eventql/util/cli/CLI.h"
#include "eventql/util/cli/term.h"
#include "eventql/util/csv/CSVInputStream.h"
#include "eventql/util/csv/BinaryCSVOutputStream.h"
#include "eventql/util/io/file.h"
#include "eventql/util/inspect.h"
#include "eventql/util/human.h"
#include "eventql/util/UTF8.h"
#include "eventql/util/http/httpclient.h"
#include "eventql/util/util/SimpleRateLimit.h"
#include "eventql/util/protobuf/MessageSchema.h"

using namespace stx;

void run(const cli::FlagParser& flags) {
  stx::Term term;

  auto table_name = flags.getString("table_name");
  auto input_file = flags.getString("input_file");
  auto api_token = flags.getString("api_token");
  auto api_host = flags.getString("api_host");
  auto shard_size = flags.getInt("shard_size");
  bool confirm_schema = !flags.isSet("skip_confirmation");

  char col_sep = '\t';
  char row_sep = '\n';
  char quote_char = '"';

  if (flags.isSet("column_separator")) {
    auto s = flags.getString("column_separator");
    if (s.size() != 1) {
      RAISEF(kIllegalArgumentError, "invalid column separator: $0", s);
    }

    col_sep = s[0];
  }

  if (flags.isSet("row_separator")) {
    auto s = flags.getString("row_separator");
    if (s.size() != 1) {
      RAISEF(kIllegalArgumentError, "invalid row separator: $0", s);
    }

    row_sep = s[0];
  }

  if (flags.isSet("quote_char")) {
    auto s = flags.getString("quote_char");
    if (s.size() != 1) {
      RAISEF(kIllegalArgumentError, "invalid quote char: $0", s);
    }

    quote_char = s[0];
  }

  stx::logInfo("dx-csv-upload", "Opening CSV file '$0'", input_file);

  auto is = FileInputStream::openFile(input_file);
  auto bom = is->readByteOrderMark();
  switch (bom) {
    case FileInputStream::BOM_UTF16:
      RAISE(kNotYetImplementedError, "UTF-16 encoded files are not yet supported");

    case FileInputStream::BOM_UTF8:
    case FileInputStream::BOM_UNKNOWN:
      break;
  };

  DefaultCSVInputStream csv(std::move(is), col_sep, row_sep, quote_char);

  stx::logInfo(
      "dx-csv-upload",
      "Analyzing the input file. This might take a few minutes...");

  Vector<String> columns;
  Vector<String> columns_dbg;
  csv.readNextRow(&columns);

  HashMap<String, HumanDataType> column_types;
  for (const auto& hdr : columns) {
    column_types[hdr] = HumanDataType::UNKNOWN;
  }

  Vector<String> row;
  size_t num_rows = 0;
  size_t num_rows_uploaded = 0;
  while (csv.readNextRow(&row)) {
    ++num_rows;

    for (size_t i = 0; i < row.size() && i < columns.size(); ++i) {
      auto& ctype = column_types[columns[i]];
      ctype = Human::detectDataTypeSeries(row[i], ctype);
    }
  }

  Vector<msg::MessageSchemaField> schema_fields;
  size_t field_num = 0;
  for (const auto& col : columns) {
    switch (column_types[col]) {

        case HumanDataType::UNSIGNED_INTEGER:
          columns_dbg.emplace_back(col + " [uint]");
          schema_fields.emplace_back(
              ++field_num,
              col,
              msg::FieldType::UINT64,
              0,
              false,
              false);
          break;

        case HumanDataType::UNSIGNED_INTEGER_NULLABLE:
          columns_dbg.emplace_back(col + " [uint]");
          schema_fields.emplace_back(
              ++field_num,
              col,
              msg::FieldType::UINT64,
              0,
              false,
              true);
          break;

        case HumanDataType::SIGNED_INTEGER:
        case HumanDataType::FLOAT:
          columns_dbg.emplace_back(col + " [float]");
          schema_fields.emplace_back(
              ++field_num,
              col,
              msg::FieldType::DOUBLE,
              0,
              false,
              false);
          break;

        case HumanDataType::SIGNED_INTEGER_NULLABLE:
        case HumanDataType::FLOAT_NULLABLE:
          columns_dbg.emplace_back(col + " [float]");
          schema_fields.emplace_back(
              ++field_num,
              col,
              msg::FieldType::DOUBLE,
              0,
              false,
              true);
          break;


        case HumanDataType::BOOLEAN:
          columns_dbg.emplace_back(col + " [boolean]");
          schema_fields.emplace_back(
              ++field_num,
              col,
              msg::FieldType::BOOLEAN,
              0,
              false,
              false);
          break;

        case HumanDataType::BOOLEAN_NULLABLE:
          columns_dbg.emplace_back(col + " [boolean]");
          schema_fields.emplace_back(
              ++field_num,
              col,
              msg::FieldType::BOOLEAN,
              0,
              false,
              true);
          break;

        case HumanDataType::DATETIME:
        case HumanDataType::DATETIME_NULLABLE:
          columns_dbg.emplace_back(col + " [datetime]");
          schema_fields.emplace_back(
              ++field_num,
              col,
              msg::FieldType::DATETIME,
              0,
              false,
              true);
          break;

        case HumanDataType::URL:
        case HumanDataType::URL_NULLABLE:
        case HumanDataType::CURRENCY:
        case HumanDataType::CURRENCY_NULLABLE:
        case HumanDataType::TEXT:
        case HumanDataType::NULL_OR_EMPTY:
          columns_dbg.emplace_back(col + " [string]");
          schema_fields.emplace_back(
              ++field_num,
              col,
              msg::FieldType::STRING,
              0,
              false,
              true);
          break;

        case HumanDataType::UNKNOWN:
        case HumanDataType::BINARY:
          RAISEF(kTypeError, "invalid column type for column '$0'", col);

    }
  }

  csv.rewind();

  auto schema = mkRef(new msg::MessageSchema("<anonymous>", schema_fields));
  stx::logInfo(
      "dx-csv-upload",
      "Found $0 row(s) and $1 column(s):\n    - $2",
      num_rows,
      columns.size(),
      StringUtil::join(columns_dbg, "\n    - "));

  if (confirm_schema) {
    stx::logInfo("dx-csv-upload", "Is this information correct? [y/n]");
    if (!term.readConfirmation()) {
      stx::logInfo("dx-csv-upload", "Aborting...");
      return;
    }
  }

  auto num_shards = (num_rows + shard_size - 1) / shard_size;
  stx::logDebug("dx-csv-upload", "Splitting into $0 shards", num_shards);

  http::HTTPClient http_client(nullptr);
  http::HTTPMessage::HeaderList auth_headers;
  auth_headers.emplace_back(
      "Authorization",
      StringUtil::format("Token $0", api_token));

  /* build create table request */
  Buffer create_req;
  {
    json::JSONOutputStream j(BufferOutputStream::fromBuffer(&create_req));
    j.beginObject();
    j.addObjectEntry("table_name");
    j.addString(table_name);
    j.addComma();
    j.addObjectEntry("update");
    j.addTrue();
    j.addComma();
    j.addObjectEntry("num_shards");
    j.addInteger(num_shards);
    j.addComma();
    j.addObjectEntry("schema");
    schema->toJSON(&j);
    j.endObject();
  }

  auto create_res = http_client.executeRequest(
      http::HTTPRequest::mkPost(
          "http://" + api_host + "/api/v1/tables/create_table",
          create_req,
          auth_headers));

  if (create_res.statusCode() != 201) {
    RAISE(kRuntimeError, create_res.body().toString());
  }

  /* status line */
  util::SimpleRateLimitedFn status_line(
      kMicrosPerSecond,
      [&num_rows_uploaded, num_rows] () {
    stx::logInfo(
        "dx-csv-upload",
        "[$0%] Uploading... $1/$2 rows",
        (size_t) ((num_rows_uploaded / (double) num_rows) * 100),
        num_rows_uploaded,
        num_rows);
  });

  csv.skipNextRow();

  for (size_t nshard = 0; num_rows_uploaded < num_rows; ++nshard) {
    Buffer shard_data;
    BinaryCSVOutputStream shard_csv(
        BufferOutputStream::fromBuffer(&shard_data));

    shard_csv.appendRow(columns);

    size_t num_rows_shard = 0;
    while (num_rows_shard < shard_size && csv.readNextRow(&row)) {
      ++num_rows_uploaded;
      ++num_rows_shard;

      for (const auto& c : row) {
        if (!UTF8::isValidUTF8(c)) {
          RAISEF(kRuntimeError, "not a valid UTF8 string: '$0'", c);
        }
      }

      shard_csv.appendRow(row);
      status_line.runMaybe();
    }

    auto upload_uri = StringUtil::format(
        "http://$0/api/v1/tables/upload_table?table=$1&shard=$2",
        api_host,
        URI::urlEncode(table_name),
        nshard);

    auto upload_res = http_client.executeRequest(
        http::HTTPRequest::mkPost(
            upload_uri,
            shard_data,
            auth_headers));

    if (upload_res.statusCode() != 201) {
      RAISE(kRuntimeError, upload_res.body().toString());
    }
  }

  status_line.runForce();
  stx::logInfo("dx-csv-upload", "Upload finished successfully :)");
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

  flags.defineFlag(
      "input_file",
      stx::cli::FlagParser::T_STRING,
      true,
      "i",
      NULL,
      "input CSV file",
      "<filename>");

  flags.defineFlag(
      "table_name",
      stx::cli::FlagParser::T_STRING,
      true,
      "t",
      NULL,
      "table name",
      "<name>");

  flags.defineFlag(
      "column_separator",
      stx::cli::FlagParser::T_STRING,
      false,
      NULL,
      "\t",
      "CSV column separator (default: '\\t')",
      "<char>");

  flags.defineFlag(
      "row_separator",
      stx::cli::FlagParser::T_STRING,
      false,
      NULL,
      "\n",
      "CSV row separator (default: '\\n')",
      "<char>");

  flags.defineFlag(
      "quote_char",
      stx::cli::FlagParser::T_STRING,
      false,
      NULL,
      "\"",
      "CSV quote char (default: '\"')",
      "<char>");

  flags.defineFlag(
      "api_token",
      stx::cli::FlagParser::T_STRING,
      true,
      "x",
      NULL,
      "DeepAnalytics API Token",
      "<token>");

  flags.defineFlag(
      "api_host",
      stx::cli::FlagParser::T_STRING,
      false,
      NULL,
      "api.eventql.io",
      "DeepAnalytics API Host",
      "<host>");

  flags.defineFlag(
      "shard_size",
      stx::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "262144",
      "shard size",
      "<num>");

  flags.defineFlag(
      "skip_confirmation",
      stx::cli::FlagParser::T_SWITCH,
      false,
      "y",
      NULL,
      "yes to all",
      "<switch>");

  flags.parseArgv(argc, argv);

  Logger::get()->setMinimumLogLevel(
      strToLogLevel(flags.getString("loglevel")));

  try {
    run(flags);
  } catch (const StandardException& e) {
    stx::logError("dx-csv-upload", "[FATAL ERROR] $0", e.what());
  }

  return 0;
}
