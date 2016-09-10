/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *   - Laura Schlimmer <laura@eventql.io>
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
#include <eventql/cli/console.h>
#include <eventql/util/inspect.h>
#include "eventql/util/logging.h"
#include "eventql/util/random.h"
#include "eventql/util/util/Base64.h"
#include "eventql/util/thread/eventloop.h"
#include "eventql/util/thread/threadpool.h"
#include "eventql/util/thread/FixedSizeThreadPool.h"
#include "eventql/util/io/TerminalOutputStream.h"
#include "eventql/util/wallclock.h"
#include "eventql/util/json/json.h"
#include "eventql/util/json/jsonrpc.h"
#include "eventql/util/http/httpclient.h"
#include "eventql/util/http/HTTPSSEResponseHandler.h"
#include "eventql/util/cli/CLI.h"
#include "eventql/util/cli/flagparser.h"
#include "eventql/util/cli/term.h"
#include "eventql/server/sql/codec/binary_codec.h"
#include "eventql/sql/result_list.h"
#include "eventql/sql/parser/tokenize.h"
#include "linenoise/linenoise.h"

namespace eventql {
namespace cli {

void Console::startInteractiveShell() {
  auto history_path = cfg_.getHistoryPath();
  if (!history_path.isEmpty()) {
    linenoiseHistorySetMaxLen(cfg_.getHistoryMaxSize());
    linenoiseHistoryLoad(history_path.get().c_str());
  } else {
    logWarning("evql", "history file can't be written - please specify the --history_path flag or $HOME");
  }

  char *p;
  String query;
  while ((p = linenoise(query.empty() ? "evql> " : "   -> ")) != NULL) {
    query += String(p);
    linenoiseFree(p);

    if (query == "quit") {
      return;
    }

    Vector<csql::Token> query_tokens;
    csql::tokenizeQuery(query, &query_tokens);
    bool query_complete = false;
    for (const auto& t : query_tokens) {
      if (t == csql::Token::T_SEMICOLON) {
        query_complete = true;
        break;
      }
    }

    if (!query_complete) {
      query += " ";
      continue;
    }

    runQuery(query);
    if (!history_path.isEmpty()) {
      linenoiseHistoryAdd(query.c_str());
      linenoiseHistorySave(history_path.get().c_str());
    }

    query.clear();
  }
}

Console::Console(const CLIConfig cli_cfg) : cfg_(cli_cfg), client_(nullptr) {}

Console::~Console() {
  if (client_) {
    evql_client_destroy(client_);
  }
}

ReturnCode Console::connect() {
  if (!client_) {
    client_ = evql_client_init();
    if (!client_) {
      return ReturnCode::error("ERUNTIME", "can't initialize eventql client");
    }
  }

  if (!cfg_.getUser().isEmpty()) {
    std::string akey = "user";
    std::string aval = cfg_.getUser().get();
    evql_client_setauth(
        client_,
        akey.data(),
        akey.size(),
        aval.data(),
        aval.size(),
        0);
  }

  if (!cfg_.getPassword().isEmpty()) {
    std::string akey = "password";
    std::string aval = cfg_.getPassword().get();
    evql_client_setauth(
        client_,
        akey.data(),
        akey.size(),
        aval.data(),
        aval.size(),
        0);
  }

  if (!cfg_.getAuthToken().isEmpty()) {
    std::string akey = "auth_token";
    std::string aval = cfg_.getAuthToken().get();
    evql_client_setauth(
        client_,
        akey.data(),
        akey.size(),
        aval.data(),
        aval.size(),
        0);
  }

  std::string database;
  bool switch_database = false;
  if (!cfg_.getDatabase().isEmpty()) {
    switch_database = true;
    database = cfg_.getDatabase().get();
  }

  auto rc = evql_client_connect(
      client_,
      cfg_.getHost().c_str(),
      cfg_.getPort(),
      switch_database ? database.c_str() : nullptr,
      0);

  if (rc < 0) {
    return ReturnCode::error("EIO", "%s", evql_client_geterror(client_));
  }

  return ReturnCode::success();
}

void Console::close() {
  if (client_) {
    evql_client_close(client_);
  }
}

Status Console::runQuery(const String& query) {
  auto stdout_os = TerminalOutputStream::fromStream(OutputStream::getStdout());
  auto stderr_os = TerminalOutputStream::fromStream(OutputStream::getStderr());
  bool line_dirty = false;
  bool is_tty = stderr_os->isTTY();
  bool batchmode = cfg_.getBatchMode();

  //if (!cfg_.getQuietMode()) {
  //  res_parser->onProgress([&stdout_os, &line_dirty, is_tty] (
  //      const csql::ExecutionStatus& status) {
  //    auto status_line = StringUtil::format(
  //        "Query running: $0%",
  //        status.progress * 100);

  //    if (is_tty) {
  //      stdout_os->eraseLine();
  //      stdout_os->print("\r" + status_line);
  //      line_dirty = true;
  //    } else {
  //      stdout_os->print(status_line + "\n");
  //    }

  //  });
  //}

  int rc = evql_query(client_, query.c_str(), NULL, 0);

  csql::ResultList results;
  std::vector<std::string> result_columns;
  size_t result_ncols;
  if (rc == 0) {
    rc = evql_num_columns(client_, &result_ncols);
  }

  for (int i = 0; rc == 0 && i < result_ncols; ++i) {
    const char* colname;
    size_t colname_len;
    rc = evql_column_name(client_, i, &colname, &colname_len);
    if (rc == -1) {
      break;
    }

    result_columns.emplace_back(colname, colname_len);
  }

  if (rc == 0) {
    if (batchmode) {
      for (const auto col : result_columns) {
        stdout_os->print(col + "\t");
      }
      stdout_os->print("\n");
    } else {
      results.addHeader(result_columns);
    }
  }

  size_t result_nrows = 0;
  while (rc >= 0) {
    const char** fields;
    size_t* field_lens;
    rc = evql_fetch_row(client_, &fields, &field_lens);
    if (rc < 1) {
      break;
    }

    ++result_nrows;
    if (batchmode) {
      for (int i = 0; i < result_ncols; ++i) {
        stdout_os->print(std::string(fields[i], field_lens[i]));
        stdout_os->print("\t");
      }
      stdout_os->print("\n");
    } else {
      std::vector<std::string> row;
      for (int i = 0; i < result_ncols; ++i) {
        row.emplace_back(fields[i], field_lens[i]);
      }

      results.addRow(row);
    }
  }

  evql_client_releasebuffers(client_);

  if (is_tty) {
    stderr_os->eraseLine();
    stderr_os->print("\r");
  }

  if (rc == -1) {
    stderr_os->print(
          "ERROR:",
          { TerminalStyle::RED, TerminalStyle::UNDERSCORE });

    stderr_os->print(" ");
    stderr_os->print(evql_client_geterror(client_));
    stderr_os->print("\n");

    return Status(eIOError);
  } else {
    if (!batchmode && results.getNumRows() > 0) {
      results.debugPrint();
    }

    String status_line = "";
    if (result_nrows > 0) {
      status_line = StringUtil::format(
          "$0 row$1 returned",
          result_nrows,
          result_nrows > 1 ? "s" : "");
    } else if (result_ncols > 0) {
      status_line = "Empty Set";
    } else {
      status_line = "Query OK";
    }

    if (!cfg_.getQuietMode()) {
      if (is_tty) {
        stderr_os->print("\r" + status_line + "\n\n");
      } else {
        stderr_os->print(status_line + "\n\n");
      }
    }

    return Status::success();
  }
}

Status Console::sendRequest(const String& query, csql::BinaryResultParser* res_parser) {
  try {
    auto url = StringUtil::format(
        "http://$0:$1/api/v1/sql",
        cfg_.getHost(),
        cfg_.getPort());

    auto db = cfg_.getDatabase().isEmpty() ? "" : cfg_.getDatabase().get();
    auto postdata = StringUtil::format(
          "format=binary&query=$0&database=$1",
          URI::urlEncode(query),
          URI::urlEncode(db));

    http::HTTPMessage::HeaderList auth_headers;
    if (!cfg_.getAuthToken().isEmpty()) {
      auth_headers.emplace_back(
          "Authorization",
          StringUtil::format("Token $0", cfg_.getAuthToken().get()));
    } else if (!cfg_.getUser().isEmpty() && !cfg_.getPassword().isEmpty()) {
      auth_headers.emplace_back(
          "Authorization",
          StringUtil::format("Basic $0",
              util::Base64::encode(
                  cfg_.getUser().get() + ":" + cfg_.getPassword().get())));
    }

    http::HTTPClient http_client(nullptr);
    auto req = http::HTTPRequest::mkPost(url, postdata, auth_headers);
    auto res = http_client.executeRequest(
        req,
        http::StreamingResponseHandler::getFactory(
            std::bind(
                &csql::BinaryResultParser::parse,
                res_parser,
                std::placeholders::_1,
                std::placeholders::_2)));

    if (!res_parser->eof()) {
      return Status(eIOError, "connection to server lost");
    }

    return Status::success();

  } catch (const StandardException& e) {
    return Status(eIOError, StringUtil::format(" $0\n", e.what()));
  }
}

Status Console::runJS(const String& program_source) {
  auto stdout_os = OutputStream::getStdout();
  auto stderr_os = TerminalOutputStream::fromStream(OutputStream::getStderr());

  try {

    bool finished = false;
    bool error = false;
    String error_string;
    bool line_dirty = false;
    bool is_tty = stderr_os->isTTY();
    String status_line;

    auto event_handler = [&] (const http::HTTPSSEEvent& ev) {
      if (ev.name.isEmpty()) {
        return;
      }

      if (ev.name.get() == "status") {
        auto obj = json::parseJSON(ev.data);
        auto tasks_completed = json::objectGetUInt64(obj, "num_tasks_completed");
        auto tasks_total = json::objectGetUInt64(obj, "num_tasks_total");
        auto tasks_running = json::objectGetUInt64(obj, "num_tasks_running");
        auto progress = json::objectGetFloat(obj, "progress");

        status_line = StringUtil::format(
            "[$0/$1] $2 tasks running ($3%)",
            tasks_completed.isEmpty() ? 0 : tasks_completed.get(),
            tasks_total.isEmpty() ? 0 : tasks_total.get(),
            tasks_running.isEmpty() ? 0 : tasks_running.get(),
            progress.isEmpty() ? 0 : progress.get() * 100);

        if (is_tty) {
          stderr_os->eraseLine();
          stderr_os->print("\r" + status_line);
          line_dirty = true;
        } else {
          stderr_os->print(status_line + "\n");
        }

        return;
      }

      bool line_erased = false;
      if (line_dirty) {
        stderr_os->eraseLine();
        stderr_os->print("\r");
        line_dirty = false;
        line_erased = true;
      }

      if (ev.name.get() == "job_started") {
        //stderr_os->printYellow(">> Job started\n");
        return;
      }

      if (ev.name.get() == "job_finished") {
        finished = true;
        return;
      }

      if (ev.name.get() == "error") {
        error = true;
        error_string = URI::urlDecode(ev.data);
        return;
      }

      if (ev.name.get() == "result") {
        stdout_os->write(URI::urlDecode(ev.data) + "\n");
        return;
      }

      if (ev.name.get() == "log") {
        stderr_os->print(URI::urlDecode(ev.data) + "\n");

        if (line_erased) {
          stderr_os->print(status_line);
          line_dirty = true;
        }

        return;
      }

    };

    auto url = StringUtil::format(
        "http://$0:$1/api/v1/mapreduce/execute",
        cfg_.getHost(),
        cfg_.getPort());

    if (!cfg_.getDatabase().isEmpty()) {
      url += StringUtil::format(
          "?database=$0",
          URI::urlEncode(cfg_.getDatabase().get()));
    }

    http::HTTPMessage::HeaderList auth_headers;
    if (!cfg_.getAuthToken().isEmpty()) {
      auth_headers.emplace_back(
          "Authorization",
          StringUtil::format("Token $0", cfg_.getAuthToken().get()));
    } else if (!cfg_.getUser().isEmpty() && !cfg_.getPassword().isEmpty()) {
      auth_headers.emplace_back(
          "Authorization",
          StringUtil::format("Basic $0",
              util::Base64::encode(
                  cfg_.getUser().get() + ":" + cfg_.getPassword().get())));
    }

    if (is_tty) {
      stderr_os->print("Launching job...");
      line_dirty = true;
    } else {
      stderr_os->print("Launching job...\n");
    }

    http::HTTPClient http_client(nullptr);
    auto req = http::HTTPRequest::mkPost(url, program_source, auth_headers);
    auto res = http_client.executeRequest(
        req,
        http::HTTPSSEResponseHandler::getFactory(event_handler));

    if (line_dirty) {
      stderr_os->eraseLine();
      stderr_os->print("\r");
    }

    if (res.statusCode() != 200) {
      error = true;
      error_string = "HTTP Error: " + res.body().toString();
    }

    if (!finished && !error) {
      error = true;
      error_string = "connection to server lost";
    }

    if (error) {
      stderr_os->print(
          "ERROR:",
          { TerminalStyle::RED, TerminalStyle::UNDERSCORE });

      stderr_os->print(" " + error_string + "\n");
      return Status(eIOError);
    } else {
      stderr_os->printGreen("Job successfully completed\n");
      return Status::success();
    }
  } catch (const StandardException& e) {
    stderr_os->print(
        "ERROR:",
        { TerminalStyle::RED, TerminalStyle::UNDERSCORE });

    stderr_os->print(StringUtil::format(" $0\n", e.what()));
    return Status(eIOError);
  }

}


} // namespace cli
} // namespace eventql

