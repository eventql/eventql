/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "stx/stdtypes.h"
#include "stx/application.h"
#include "stx/cli/flagparser.h"
#include "stx/cli/CLI.h"
#include "stx/csv/CSVInputStream.h"
#include "stx/csv/BinaryCSVOutputStream.h"
#include "stx/io/file.h"
#include "stx/inspect.h"
#include "stx/human.h"
#include "stx/UTF8.h"
#include "stx/http/httpclient.h"
#include "stx/util/SimpleRateLimit.h"
#include "stx/protobuf/MessageSchema.h"

using namespace stx;

void run(const cli::FlagParser& flags) {
  auto table_name = flags.getString("table_name");
  auto input_file = flags.getString("input_file");
  auto api_token = flags.getString("api_token");
  auto api_host = flags.getString("api_host");
  auto shard_size = flags.getInt("shard_size");

  stx::logInfo("dx-csv-upload", "Opening CSV file '$0'", input_file);

  auto csv = CSVInputStream::openFile(
      input_file,
      '\t',
      '\n');

  stx::logInfo(
      "dx-csv-upload",
      "Analyzing the input file. This might take a few minutes...");

  Vector<String> columns;
  csv->readNextRow(&columns);

  HashMap<String, HumanDataType> column_types;
  for (const auto& hdr : columns) {
    column_types[hdr] = HumanDataType::UNKNOWN;
  }

  Vector<String> row;
  size_t num_rows = 0;
  size_t num_rows_uploaded = 0;
  while (csv->readNextRow(&row)) {
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
          schema_fields.emplace_back(
              ++field_num,
              col,
              msg::FieldType::UINT64,
              0,
              false,
              false);
          break;

        case HumanDataType::UNSIGNED_INTEGER_NULLABLE:
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
          schema_fields.emplace_back(
              ++field_num,
              col,
              msg::FieldType::DOUBLE,
              0,
              false,
              true);
          break;


        case HumanDataType::BOOLEAN:
          schema_fields.emplace_back(
              ++field_num,
              col,
              msg::FieldType::BOOLEAN,
              0,
              false,
              false);
          break;

        case HumanDataType::BOOLEAN_NULLABLE:
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
        case HumanDataType::URL:
        case HumanDataType::URL_NULLABLE:
        case HumanDataType::CURRENCY:
        case HumanDataType::CURRENCY_NULLABLE:
        case HumanDataType::TEXT:
        case HumanDataType::NULL_OR_EMPTY:
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

  csv->rewind();

  auto schema = mkRef(new msg::MessageSchema("<anonymous>", schema_fields));
  stx::logDebug("dx-csv-upload", "Detected schema:\n$0", schema->toString());

  auto num_shards = (num_rows + shard_size - 1) / shard_size;
  stx::logDebug("dx-csv-upload", "Splitting into $0 shards", num_shards);

  http::HTTPClient http_client;
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

  csv->skipNextRow();

  for (size_t nshard = 0; num_rows_uploaded < num_rows; ++nshard) {
    Buffer shard_data;
    BinaryCSVOutputStream shard_csv(
        BufferOutputStream::fromBuffer(&shard_data));

    shard_csv.appendRow(columns);

    size_t num_rows_shard = 0;
    while (num_rows_shard < shard_size && csv->readNextRow(&row)) {
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
      "api.zbase.io"
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
