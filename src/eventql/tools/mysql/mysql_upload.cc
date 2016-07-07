/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *   - Laura Schlimmer <laura@zscale.io>
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
#include <eventql/eventql.h>
#include <eventql/util/stdtypes.h>
#include <eventql/util/application.h>
#include <eventql/util/cli/flagparser.h>
#include <eventql/util/csv/BinaryCSVOutputStream.h>
#include <eventql/util/http/httpclient.h>
#include <eventql/util/json/jsonoutputstream.h>
#include "eventql/util/thread/FixedSizeThreadPool.h"
#include "eventql/util/util/SimpleRateLimit.h"
#include <eventql/util/mysql/MySQL.h>
#include <eventql/util/mysql//MySQLConnection.h>

using namespace eventql;

void run(const cli::FlagParser& flags) {
  auto source_table = flags.getString("source_table");
  auto destination_table = flags.getString("destination_table");
  auto api_token = flags.getString("api_token");
  String api_url = "http://api.zscale.io/api/v1";
  auto shard_size = flags.getInt("shard_size");
  auto mysql_addr = flags.getString("mysql");

  logInfo("mysql-upload", "Connecting to MySQL Server...");

  util::mysql::mysqlInit();
  auto mysql_conn = util::mysql::MySQLConnection::openConnection(URI(mysql_addr));

  logInfo(
      "mysql-upload",
      "Analyzing the input table. This might take a few minutes...");

  auto schema = mysql_conn->getTableSchema(source_table);
  logDebug("mysql-upload", "Table Schema:\n$0", schema->toString());

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
    logError(
        "mysql-upload",
        "Table '$0' appears to be empty",
        source_table);

    return;
  }

  auto num_shards = (num_rows + shard_size - 1) / shard_size;
  logDebug("mysql-upload", "Splitting into $0 shards", num_shards);

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
    logInfo(
        "mysql-upload",
        "[$0%] Uploading... $1/$2 rows",
        (size_t) ((num_rows_uploaded / (double) num_rows) * 100),
        num_rows_uploaded,
        num_rows);
  });

  Vector<String> columns;
  for (const auto& field : schema->fields()) {
    columns.emplace_back(field.name);
  }

  thread::FixedSizeThreadPool tpool(thread::ThreadPoolOptions{}, 4);
  tpool.start();

  Buffer shard_data;
  BinaryCSVOutputStream shard_csv(BufferOutputStream::fromBuffer(&shard_data));
  shard_csv.appendRow(columns);

  size_t nshard = 0;
  auto upload_shard = [&] {
    logDebug(
        "mysql-upload",
        "Shard $0 ready for upload; size=$1MB",
        nshard,
        shard_data.size() / 1000000.0);

    auto upload_uri = StringUtil::format(
        "$0/tables/upload_table?table=$1&shard=$2",
        api_url,
        URI::urlEncode(destination_table),
        nshard++);

    tpool.run([upload_uri, shard_data, auth_headers] () {
      logDebug("mysql-upload", "Uploading: $0", upload_uri);

      http::HTTPClient http_client;
      auto upload_res = http_client.executeRequest(
          http::HTTPRequest::mkPost(
              upload_uri,
              shard_data,
              auth_headers));

      logDebug("mysql-upload", "Upload finished: $0", upload_uri);
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

  logInfo("dx-mysql-upload", "Upload finished successfully :)");
  exit(0);
}

int main(int argc, const char** argv) {
  Application::init();
  //Application::logToStderr();

  cli::FlagParser flags;

  flags.defineFlag(
      "loglevel",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      "INFO",
      "loglevel",
      "<level>");

  flags.defineFlag(
      "source_table",
      cli::FlagParser::T_STRING,
      true,
      "t",
      NULL,
      "table name",
      "<name>");

  flags.defineFlag(
      "destination_table",
      cli::FlagParser::T_STRING,
      true,
      "t",
      NULL,
      "table name",
      "<name>");

  flags.defineFlag(
      "api_token",
      cli::FlagParser::T_STRING,
      true,
      "x",
      NULL,
      "DeepAnalytics API Token",
      "<token>");

  flags.defineFlag(
      "mysql",
      cli::FlagParser::T_STRING,
      true,
      "x",
      "mysql://localhost:3306/mydb?user=root",
      "MySQL URI",
      "<url>");

  flags.defineFlag(
      "shard_size",
      cli::FlagParser::T_INTEGER,
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
    logError("mysql-upload", "[FATAL ERROR] $0", e.what());
  }

  return 0;
}
