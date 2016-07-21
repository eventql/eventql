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
#include <unistd.h>

using namespace eventql;

struct UploadShard {
  Buffer data;
  size_t nrows;
};

bool run(const cli::FlagParser& flags) {
  auto source_table = flags.getString("source_table");
  auto destination_table = flags.getString("destination_table");
  auto shard_size = flags.getInt("shard_size");
  auto num_upload_threads = flags.getInt("upload_threads");
  auto mysql_addr = flags.getString("mysql");
  auto host = flags.getString("host");
  auto port = flags.getInt("port");
  auto db = flags.getString("database");
  auto max_retries = flags.getInt("max_retries");

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
  util::SimpleRateLimitedFn status_line(kMicrosPerSecond, [&] () {
    logInfo(
        "mysql2evql",
        "Uploading... $0 rows",
        num_rows_uploaded.load());
  });

  /* start upload threads */
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

  auto insert_uri = StringUtil::format(
      "http://$0:$1/api/v1/tables/insert",
      host,
      port);

  bool upload_done = false;
  bool upload_error = false;
  thread::Queue<UploadShard> upload_queue(1);
  Vector<std::thread> upload_threads(num_upload_threads);
  for (size_t i = 0; i < num_upload_threads; ++i) {
    upload_threads[i] = std::thread([&] {
      while (!upload_done) {
        auto shard = upload_queue.interruptiblePop();
        if (shard.isEmpty()) {
          continue;
        }

        bool success = false;
        for (size_t retry = 0; retry < max_retries; ++retry) {
          sleep(2 * retry);
        
          logDebug(
              "mysql2evql",
              "Uploading batch; target=$0:$1 size=$2MB",
              host,
              port,
              shard.get().data.size() / 1000000.0);

          try {
            http::HTTPClient http_client;
            auto upload_res = http_client.executeRequest(
                http::HTTPRequest::mkPost(
                    insert_uri,
                    "[" + shard.get().data.toString() + "]",
                    auth_headers));

            if (upload_res.statusCode() != 201) {
              logError(
                  "mysql2evql", "[FATAL ERROR]: HTTP Status Code $0 $1",
                  upload_res.statusCode(),
                  upload_res.body().toString());
                  
              if (upload_res.statusCode() == 403) {
                break;
              } else {
                continue;
              }
            }

            num_rows_uploaded += shard.get().nrows;
            status_line.runMaybe();
            success = true;
            break;
          } catch (const std::exception& e) {
            logError("mysql2evql", e, "error while uploading table data");
          }
        }
        
        if (!success) {
          upload_error = true;
        }
      }
    });
  }

  /* fetch rows from mysql */
  UploadShard shard;
  shard.nrows = 0;
  json::JSONOutputStream json(BufferOutputStream::fromBuffer(&shard.data));

  String where_expr;
  if (flags.isSet("filter")) {
    where_expr = "WHERE " + flags.getString("filter");
  }

  auto get_rows_qry = StringUtil::format(
      "SELECT * FROM `$0` $1;",
     source_table,
     where_expr);

  mysql_conn->executeQuery(
      get_rows_qry,
      [&] (const Vector<String>& column_values) -> bool {
    ++shard.nrows;

    logTrace("mysql2evql", "Uploading row: $0", inspect(column_values));

    if (shard.nrows > 1) {
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

    if (shard.nrows == shard_size) {
      upload_queue.insert(shard, true);
      shard.data.clear();
      shard.nrows = 0;
    }

    status_line.runMaybe();
    return !upload_error;
  });

  if (!upload_error) {
    if (shard.nrows > 0) {
      upload_queue.insert(shard, true);
    }

    upload_queue.waitUntilEmpty();
  }

  upload_done = true;
  upload_queue.wakeup();
  for (auto& t : upload_threads) {
    t.join();
  }

  status_line.runForce();

  if (upload_error) {
    logInfo("mysql2evql", "Upload finished with errors");
    return false;
  } else {
    logInfo("mysql2evql", "Upload finished successfully :)");
    return true;
  }
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
      "filter",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "boolean sql expression",
      "<sql_expr>");


  flags.defineFlag(
      "shard_size",
      cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "128",
      "shard size",
      "<num>");

  flags.defineFlag(
      "upload_threads",
      cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "8",
      "concurrent uploads",
      "<num>");

  flags.defineFlag(
      "max_retries",
      cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "20",
      "max number of upload retries",
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
    if (run(flags)) {
      return 0;
    } else {
      return 1;
    }
  } catch (const StandardException& e) {
    logFatal("mysql2evql", "$0", e.what());
    return 1;
  }
}
