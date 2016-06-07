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
#include "linenoise/linenoise.h"

namespace eventql {
namespace cli {

void Console::startInteractiveShell() {
  char *p;
  while ((p = linenoise("evql> ")) != NULL) {
    String line(p);
    linenoiseFree(p);

    if (line == "quit") {
      return;

    } else {
      runQuery(line);
    }
  }
}

Console::Console(const CLIConfig cli_cfg) : cfg_(cli_cfg) {}

Status Console::runQuery(const String& query) {
  if (cfg_.getBatchMode()) {
    return runQueryBatch(query);
  } else {
    return runQueryTable(query);
  }
}

Status Console::runQueryTable(const String& query) {
  auto stdout_os = OutputStream::getStdout();
  auto stderr_os = TerminalOutputStream::fromStream(OutputStream::getStderr());
  bool line_dirty = false;
  bool is_tty = stderr_os->isTTY();

  bool error = false;
  csql::ResultList results;
  auto res_parser = new csql::BinaryResultParser();

  res_parser->onProgress([&stderr_os, &line_dirty, is_tty] (
      const csql::ExecutionStatus& status) {
    auto status_line = StringUtil::format(
        "Query running: $0%",
        status.progress * 100);

    if (is_tty) {
      stderr_os->eraseLine();
      stderr_os->print("\r" + status_line);
      line_dirty = true;
    } else {
      stderr_os->print(status_line + "\n");
    }

  });

  res_parser->onTableHeader([&results] (const Vector<String>& columns) {
    results.addHeader(columns);
  });

  res_parser->onRow([&results] (int argc, const csql::SValue* argv) {
    results.addRow(argv, argc);
  });

  res_parser->onError([&stderr_os, &error] (const String& error_str) {
    stderr_os->print(
        "ERROR:",
        { TerminalStyle::RED, TerminalStyle::UNDERSCORE });

    stderr_os->print(StringUtil::format(" $0\n", error_str));
    error = true;
  });

  auto res = sendRequest(query, res_parser);
  if (!res.isSuccess()) {
    stderr_os->print(
          "ERROR:",
          { TerminalStyle::RED, TerminalStyle::UNDERSCORE });

    stderr_os->print(res.message());
    return res;
  }

  if (is_tty) {
    stderr_os->eraseLine();
    stderr_os->print("\r");
  }

  if (error) {
    return Status(eIOError);
  }

  if (results.getNumRows() > 0) {
    results.debugPrint();

    auto num_rows = results.getNumRows();
    auto status_line = StringUtil::format(
        "$0 row$1 returned",
        num_rows,
        num_rows > 1 ? "s" : "");

    if (is_tty) {
      stderr_os->print("\r" + status_line + "\n\n");
    } else {
      stderr_os->print(status_line + "\n\n");
    }

  } else {
    String line = is_tty ? "\r" : "";
    if (results.getNumColumns() == 0) {
      stderr_os->print(line + "Query OK \n\n");
    } else {
      stderr_os->print(line + "Empty set \n\n");
    }
  }

  return Status::success();
}

Status Console::runQueryBatch(const String& query) {
  auto stdout_os = OutputStream::getStdout();
  auto stderr_os = TerminalOutputStream::fromStream(OutputStream::getStderr());
  bool line_dirty = false;
  bool is_tty = stderr_os->isTTY();

  bool error = false;
  csql::ResultList results;
  auto res_parser = new csql::BinaryResultParser();

  res_parser->onTableHeader([&stderr_os] (const Vector<String>& columns) {
    for (const auto col : columns) {
      stderr_os->print(col + "\t");
    }
    stderr_os->print("\n");
  });

  res_parser->onRow([&stderr_os] (int argc, const csql::SValue* argv) {
    for (size_t i = 0; i < argc; ++i) {
      stderr_os->print(argv[i].getString() + "\t");
    }
    stderr_os->print("\n");
    //results->addRow(argv, argc);
  });

  res_parser->onError([&stderr_os, &error] (const String& error_str) {
    stderr_os->print(
        "ERROR:",
        { TerminalStyle::RED, TerminalStyle::UNDERSCORE });

    stderr_os->print(StringUtil::format(" $0\n", error_str));
    error = true;
  });

  auto res = sendRequest(query, res_parser);
  if (!res.isSuccess()) {
    stderr_os->print(
          "ERROR:",
          { TerminalStyle::RED, TerminalStyle::UNDERSCORE });

    stderr_os->print(res.message());
    return res;
  }

  if (is_tty) {
    stderr_os->eraseLine();
    stderr_os->print("\r");
  }

  if (error) {
    return Status(eIOError);
  }

  return Status::success();
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
    } else if (!cfg_.getPassword().isEmpty()) {
      auth_headers.emplace_back(
          "Authorization",
          StringUtil::format("Basic $0",
              util::Base64::encode(
                  cfg_.getUser() + ":" + cfg_.getPassword().get())));
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

        auto status_line = StringUtil::format(
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

      if (line_dirty) {
        stderr_os->eraseLine();
        stderr_os->print("\r");
        line_dirty = false;
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
        return;
      }

    };

    auto url = StringUtil::format(
        "http://$0:$1/api/v1/mapreduce/execute",
        cfg_.getHost(),
        cfg_.getPort());

    http::HTTPMessage::HeaderList auth_headers;
    auth_headers.emplace_back(
        "Authorization",
        StringUtil::format("Token $0", cfg_.getAuthToken().get()));

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

