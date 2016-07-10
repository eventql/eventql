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
  auto shard_size = flags.getInt("shard_size");
  auto mysql_addr = flags.getString("mysql");
  auto host = flags.getString("host");
  auto port = flags.getInt("port");
  auto db = flags.getString("database");

  logInfo("evql-mysql-import", "Connecting to MySQL Server...");

  util::mysql::mysqlInit();
  auto mysql_conn = util::mysql::MySQLConnection::openConnection(URI(mysql_addr));

  logInfo(
      "evql-mysql-import",
      "Analyzing the input table. This might take a few minutes...");

  auto schema = mysql_conn->getTableSchema(source_table);
  logDebug("evql-mysql-import", "Table Schema:\n$0", schema->toString());
  Vector<String> column_names;
  for (const auto& field : schema->fields()) {
    column_names.emplace_back(field.name);
  }

  /* count rows */
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
        "evql-mysql-import",
        "Table '$0' appears to be empty",
        source_table);
    return;
  }

  auto num_shards = (num_rows + shard_size - 1) / shard_size;
  logDebug("evql-mysql-import", "Splitting into $0 shards", num_shards);

  /* status line */
  util::SimpleRateLimitedFn status_line(
      kMicrosPerSecond,
      [&num_rows_uploaded, num_rows] () {
    logInfo(
        "evql-mysql-import",
        "[$0%] Uploading... $1/$2 rows",
        (size_t) ((num_rows_uploaded / (double) num_rows) * 100),
        num_rows_uploaded,
        num_rows);
  });

  /* upload rows */
  size_t nshard = 0;
  size_t num_rows_shard = 0;
  Buffer shard_data;
  json::JSONOutputStream json(BufferOutputStream::fromBuffer(&shard_data));

  auto get_rows_qry = StringUtil::format("SELECT * FROM `$0`;", source_table);
  mysql_conn->executeQuery(
      get_rows_qry,
      [&] (const Vector<String>& column_values) -> bool {
    ++num_rows_shard;
    ++num_rows_uploaded;

    if (num_rows_shard > 1) {
      json.addComma();
    }

    json.beginObject();
    json.addObjectEntry("database");
    json.addString(db);
    json.addComma();
    json.addObjectEntry("table");
    json.addString(destination_table);
    json.addComma();
    json.addObjectEntry("data");
    json.beginObject();

    for (size_t i = 0; i < column_names.size() && i < column_values.size(); ++i) {
      if (i > 0 ){
        json.addComma();
      }

      json.addObjectEntry(column_names[i]);
      json.addString(column_values[i]);
    }

    json.endObject();
    json.endObject();

    if (num_rows_shard == shard_size ||
        num_rows_uploaded == num_rows) {
      logDebug(
          "evql-mysql-import",
          "Uploading shard $0; size=$1MB",
          nshard + 1,
          shard_data.size() / 1000000.0);

      auto insert_uri = StringUtil::format(
          "http://$0:$1/api/v1/tables/insert",
          host,
          port);

      http::HTTPMessage::HeaderList auth_headers;
      if (flags.isSet("auth_token")) {
        auth_headers.emplace_back(
            "Authorization",
            StringUtil::format("Token $0", flags.getString("auth_token")));
      //} else if (!cfg_.getPassword().isEmpty()) {
      //  auth_headers.emplace_back(
      //      "Authorization",
      //      StringUtil::format("Basic $0",
      //          util::Base64::encode(
      //              cfg_.getUser() + ":" + cfg_.getPassword().get())));
      }

      http::HTTPClient http_client;
      auto upload_res = http_client.executeRequest(
          http::HTTPRequest::mkPost(
              insert_uri,
              "[" + shard_data.toString() + "]",
              auth_headers));

      logDebug("evql-mysql-import", "Upload finished: $0", insert_uri);
      if (upload_res.statusCode() != 201) {
        RAISE(kRuntimeError, upload_res.body().toString());
      }

      shard_data.clear();
      ++nshard;
    }

    if (num_rows_uploaded < num_rows) {
      status_line.runMaybe();
      return true;
    } else {
      return false;
    }
  });

  status_line.runForce();

  logInfo("evql-mysql-import", "Upload finished successfully :)");
  exit(0);
}

int main(int argc, const char** argv) {
  Application::init();
  Application::logToStderr("evql-mysql-import");

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
      "host",
      cli::FlagParser::T_STRING,
      false,
      "h",
      "localhost",
      "eventql server hostname",
      "<host>");

  flags.defineFlag(
      "port",
      cli::FlagParser::T_INTEGER,
      false,
      "p",
      "9175",
      "eventql server port",
      "<port>");

  flags.defineFlag(
      "database",
      cli::FlagParser::T_STRING,
      false,
      "db",
      "",
      "eventql database",
      "<database>");

  flags.defineFlag(
      "auth_token",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "auth token",
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

  try {
    flags.parseArgv(argc, argv);
  } catch (const StandardException& e) {
    logError("evql-mysql-import", "$0", e.what());
    auto stdout_os = OutputStream::getStdout();
    flags.printUsage(stdout_os.get());
    return 0;
  }

  Logger::get()->setMinimumLogLevel(
      strToLogLevel(flags.getString("loglevel")));

  try {
    run(flags);
  } catch (const StandardException& e) {
    logError("evql-mysql-import", "[FATAL ERROR] $0", e.what());
  }

  return 0;
}
