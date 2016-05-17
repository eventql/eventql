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

namespace eventql {
namespace cli {

extern "C" {
extern char *readline(char *prompt);
}

void Console::startInteractiveShell() {
  char *p;

  while ((p = readline("evql> ")) != NULL) {
    runQuery(p);
    free(p);
  }
}

Console::Console(const ConsoleOptions& options) : cfg_(options) {}

Status Console::runQuery(const String& query) {
  auto stdout_os = OutputStream::getStdout();
  auto stderr_os = TerminalOutputStream::fromStream(OutputStream::getStderr());

  bool error = false;
  csql::BinaryResultParser res_parser;

  res_parser.onTableHeader([] (const Vector<String>& columns) {
    iputs("columns: $0", columns);
  });

  res_parser.onRow([] (int argc, const csql::SValue* argv) {
    iputs("row: $0", Vector<csql::SValue>(argv, argv + argc));
  });

  res_parser.onError([&stderr_os, &error] (const String& error_str) {
    stderr_os->print(
        "ERROR:",
        { TerminalStyle::RED, TerminalStyle::UNDERSCORE });

    stderr_os->print(StringUtil::format(" $0\n", error_str));
    error = true;
  });

  try {
    bool is_tty = stderr_os->isTTY();

    auto url = StringUtil::format(
        "http://$0:$1/api/v1/sql",
        cfg_.server_host,
        cfg_.server_port);

    auto postdata = StringUtil::format(
          "format=binary&query=$0",
          URI::urlEncode(query));

    http::HTTPMessage::HeaderList auth_headers;
    auth_headers.emplace_back(
        "Authorization",
        StringUtil::format("Token $0", cfg_.server_auth_token));

    http::HTTPClient http_client(nullptr);
    auto req = http::HTTPRequest::mkPost(url, postdata, auth_headers);
    auto res = http_client.executeRequest(
        req,
        http::StreamingResponseHandler::getFactory(
            std::bind(
                &csql::BinaryResultParser::parse,
                &res_parser,
                std::placeholders::_1,
                std::placeholders::_2)));

    if (!res_parser.eof()) {
      stderr_os->print(
          "ERROR:",
          { TerminalStyle::RED, TerminalStyle::UNDERSCORE });

      stderr_os->print(" connection to server lost");
      return Status(eIOError);
    }
  } catch (const StandardException& e) {
    stderr_os->print(
        "ERROR:",
        { TerminalStyle::RED, TerminalStyle::UNDERSCORE });

    stderr_os->print(StringUtil::format(" $0\n", e.what()));
    return Status(eIOError);
  }

  if (error) {
    return Status::success();
  } else {
    return Status(eIOError);
  }
}

} // namespace cli
} // namespace eventql

