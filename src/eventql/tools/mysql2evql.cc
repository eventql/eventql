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
#include <eventql/util/mysql/MySQLConnection.h>

using namespace eventql;

void uploadTable(
    const String& host,
    const uint64_t port,
    http::HTTPMessage::HeaderList* auth_headers,
    Buffer* shard_data) {
  logDebug(
      "mysql2evql",
      "Uploading batch; target=$0:$1 size=$2MB",
      host,
      port,
      shard_data->size() / 1000000.0);

  auto insert_uri = StringUtil::format(
      "http://$0:$1/api/v1/tables/insert",
      host,
      port);

  http::HTTPClient http_client;
  auto upload_res = http_client.executeRequest(
      http::HTTPRequest::mkPost(
          insert_uri,
          "[" + shard_data->toString() + "]",
          *auth_headers));

  logDebug("mysql2evql", "Upload finished: $0", insert_uri);
  if (upload_res.statusCode() != 201) {
    logError(
        "mysql2evql", "[FATAL ERROR]: HTTP Status Code $0 $1",
        upload_res.statusCode(),
        upload_res.body().toString());
    RAISE(kRuntimeError, upload_res.body().toString());
  }
}

void run(const cli::FlagParser& flags) {
  auto source_table = flags.getString("source_table");
  auto destination_table = flags.getString("destination_table");
  auto shard_size = flags.getInt("shard_size");
  auto mysql_addr = flags.getString("mysql");
  auto host = flags.getString("host");
  auto port = flags.getInt("port");
  auto db = flags.getString("database");

  logInfo("mysql2evql", "Connecting to MySQL Server...");

  util::mysql::mysqlInit();
  auto mysql_conn = util::mysql::MySQLConnection::openConnection(URI(mysql_addr));

  logInfo(
      "mysql2evql",
      "Analyzing the input table. This might take a few minutes...");

  auto schema = mysql_conn->getTableSchema(source_table);
  logDebug("mysql2evql", "Table Schema:\n$0", schema->toString());
  Vector<String> column_names;
  for (const auto& field : schema->fields()) {
    column_names.emplace_back(field.name);
  }

  /* status line */
  std::atomic<size_t> num_rows_uploaded(0);
  util::SimpleRateLimitedFn status_line(
      kMicrosPerSecond,
      [&num_rows_uploaded] () {
    logInfo(
        "mysql2evql",
        "Uploading... $0 rows",
        num_rows_uploaded.load());
  });

  /* upload rows */
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

  Buffer shard_data;
  size_t num_rows_shard = 0;
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

    if (num_rows_shard == shard_size) {
      uploadTable(host, port, &auth_headers, &shard_data);
      shard_data.clear();
      num_rows_shard = 0;
    }

    status_line.runMaybe();
    return true;
  });

  if (num_rows_shard > 0) {
    uploadTable(host, port, &auth_headers, &shard_data);
  }

  status_line.runForce();
  logInfo("mysql2evql", "Upload finished successfully :)");
}

int main(int argc, const char** argv) {
  Application::init();
  Application::logToStderr("mysql2evql");

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
      true,
      "h",
      NULL,
      "eventql server hostname",
      "<host>");

  flags.defineFlag(
      "port",
      cli::FlagParser::T_INTEGER,
      true,
      "p",
      NULL,
      "eventql server port",
      "<port>");

  flags.defineFlag(
      "database",
      cli::FlagParser::T_STRING,
      true,
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
      "MySQL connection string",
      "<url>");

  flags.defineFlag(
      "shard_size",
      cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "128",
      "shard size",
      "<num>");

  try {
    flags.parseArgv(argc, argv);
  } catch (const StandardException& e) {
    logError("mysql2evql", "$0", e.what());
    auto stdout_os = OutputStream::getStdout();
    flags.printUsage(stdout_os.get());
    return 0;
  }

  Logger::get()->setMinimumLogLevel(
      strToLogLevel(flags.getString("loglevel")));

  try {
    run(flags);
    return 0;
  } catch (const StandardException& e) {
    return 1;
  }
}
