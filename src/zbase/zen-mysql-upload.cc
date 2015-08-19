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
#include "stx/thread/FixedSizeThreadPool.h"
#include "stx/http/httpclient.h"
#include "stx/util/SimpleRateLimit.h"
#include "stx/protobuf/MessageSchema.h"
#include "zbase/util/mysql//MySQL.h"
#include "zbase/util/mysql//MySQLConnection.h"

using namespace stx;

void run(const cli::FlagParser& flags) {
  auto source_table = flags.getString("source_table");
  auto destination_table = flags.getString("destination_table");
  auto api_token = flags.getString("api_token");
  String api_url = "http://api.zbase.io/api/v1";
  auto shard_size = flags.getInt("shard_size");
  auto mysql_addr = flags.getString("mysql");

  stx::logInfo("dx-mysql-upload", "Connecting to MySQL Server...");

  mysql::mysqlInit();
  auto mysql_conn = mysql::MySQLConnection::openConnection(URI(mysql_addr));

  stx::logInfo(
      "dx-mysql-upload",
      "Analyzing the input table. This might take a few minutes...");

  auto schema = mysql_conn->getTableSchema(source_table);
  stx::logDebug("dx-mysql-upload", "Table Schema:\n$0", schema->toString());

  auto count_rows_qry = StringUtil::format(
      "SELECT count(*) FROM `$0`;",
      source_table);

  size_t num_rows = 0;
  size_t num_rows_uploaded = 0;
  mysql_conn->executeQuery(
      count_rows_qry,
      [&num_rows] (const Vector<String>& row) -> bool {
    if (row.size() == 1) num_rows = std::stoull(row[0]);
    return false;
  });

  if (num_rows == 0) {
    stx::logError(
        "dx-mysql-upload",
        "Table '$0' appears to be empty",
        source_table);

    return;
  }

  auto num_shards = (num_rows + shard_size - 1) / shard_size;
  stx::logDebug("dx-mysql-upload", "Splitting into $0 shards", num_shards);

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
    j.addString(destination_table);
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

  {
    http::HTTPClient http_client;
    auto create_res = http_client.executeRequest(
        http::HTTPRequest::mkPost(
            api_url + "/tables/create_table",
            create_req,
            auth_headers));

    if (create_res.statusCode() != 201) {
      RAISE(kRuntimeError, create_res.body().toString());
    }
  }

  /* status line */
  util::SimpleRateLimitedFn status_line(
      kMicrosPerSecond,
      [&num_rows_uploaded, num_rows] () {
    stx::logInfo(
        "dx-mysql-upload",
        "[$0%] Uploading... $1/$2 rows",
        (size_t) ((num_rows_uploaded / (double) num_rows) * 100),
        num_rows_uploaded,
        num_rows);
  });

  Vector<String> columns;
  for (const auto& field : schema->fields()) {
    columns.emplace_back(field.name);
  }

  thread::FixedSizeThreadPool tpool(4, 4);
  tpool.start();

  Buffer shard_data;
  BinaryCSVOutputStream shard_csv(BufferOutputStream::fromBuffer(&shard_data));
  shard_csv.appendRow(columns);

  size_t nshard = 0;
  auto upload_shard = [&] {
    stx::logDebug(
        "dx-mysql-upload",
        "Shard $0 ready for upload; size=$1MB",
        nshard,
        shard_data.size() / 1000000.0);

    auto upload_uri = StringUtil::format(
        "$0/tables/upload_table?table=$1&shard=$2",
        api_url,
        URI::urlEncode(destination_table),
        nshard++);

    tpool.run([upload_uri, shard_data, auth_headers] () {
      stx::logDebug("dx-mysql-upload", "Uploading: $0", upload_uri);

      http::HTTPClient http_client;
      auto upload_res = http_client.executeRequest(
          http::HTTPRequest::mkPost(
              upload_uri,
              shard_data,
              auth_headers));

      stx::logDebug("dx-mysql-upload", "Upload finished: $0", upload_uri);
      if (upload_res.statusCode() != 201) {
        RAISE(kRuntimeError, upload_res.body().toString());
      }
    });

    shard_data.clear();
    shard_csv.appendRow(columns);
  };

  size_t num_rows_shard = 0;
  auto get_rows_qry = StringUtil::format("SELECT * FROM `$0`;", source_table);
  mysql_conn->executeQuery(
      get_rows_qry,
      [&] (const Vector<String>& row) -> bool {
    shard_csv.appendRow(row);

    if (++num_rows_shard == shard_size) {
      upload_shard();
      num_rows_shard = 0;
    }

    if (++num_rows_uploaded < num_rows) {
      status_line.runMaybe();
      return true;
    } else {
      return false;
    }
  });

  if (num_rows_shard > 0) {
    upload_shard();
  }

  status_line.runForce();
  tpool.stop();

  stx::logInfo("dx-mysql-upload", "Upload finished successfully :)");
  exit(0);
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
      "source_table",
      stx::cli::FlagParser::T_STRING,
      true,
      "t",
      NULL,
      "table name",
      "<name>");

  flags.defineFlag(
      "destination_table",
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
      "mysql",
      stx::cli::FlagParser::T_STRING,
      true,
      "x",
      "mysql://localhost:3306/mydb?user=root",
      "MySQL URI",
      "<url>");

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
    stx::logError("dx-mysql-upload", "[FATAL ERROR] $0", e.what());
  }

  return 0;
}
