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
#include "eventql/util/csv/CSVInputStream.h"
#include "eventql/util/csv/BinaryCSVOutputStream.h"
#include "eventql/util/io/file.h"
#include "eventql/util/inspect.h"
#include "eventql/util/human.h"
#include "eventql/util/stats/statsd.h"
#include "eventql/util/thread/FixedSizeThreadPool.h"
#include "eventql/util/http/httpclient.h"
#include "eventql/util/util/SimpleRateLimit.h"
#include "eventql/util/protobuf/MessageSchema.h"

#include "eventql/eventql.h"

void run(const cli::FlagParser& flags) {
  String api_url = "http://api.eventql.io/api/v1";
  if (getenv("DX_DEVEL")) {
    api_url = "http://dev.eventql.io/api/v1";
  }

  auto api_token = flags.getString("api_token");
  auto statsd_port = flags.getInt("statsd_port");
  auto prefix = flags.getString("key_prefix");

  thread::EventLoop ev;
  thread::FixedSizeThreadPool tpool(thread::ThreadPoolOptions{}, 4);
  tpool.start();

  http::HTTPConnectionPool http(&ev, nullptr);

  http::HTTPMessage::HeaderList auth_headers;
  auth_headers.emplace_back(
      "Authorization",
      StringUtil::format("Token $0", api_token));

  statsd::StatsdServer statsd_server(&ev, &tpool);
  statsd_server.onSample([&http, auth_headers, api_url, prefix] (
      const String& statsd_key,
      double value,
      const Vector<Pair<String, String>>& labels) {
    auto key = prefix + statsd_key;

    logDebug(
        "dx-statsd-sink",
        "Uploading sample: $0=$1 $2",
        key,
        value,
        inspect(labels).c_str());

    auto res = http.executeRequest(
        http::HTTPRequest::mkPost(
            StringUtil::format(
                "$0/metrics?metric=$1&value=$2",
                api_url,
                URI::urlEncode(key),
                value),
            Buffer(),
            auth_headers));

    res.wait();
    if (res.get().statusCode() != 201) {
      logError(
          "dx-statsd-sink",
          "error while uploading sample: $0",
          res.get().body().toString());
    }
  });

  logNotice("dx-statsd-sink", "Starting StatsD server on port $0", statsd_port);
  statsd_server.listen(statsd_port);
  ev.run();
}

int main(int argc, const char** argv) {
  util::Application::init();
  util::Application::logToStderr();

  util::cli::FlagParser flags;

  flags.defineFlag(
      "api_token",
      util::cli::FlagParser::T_STRING,
      true,
      "x",
      NULL,
      "DeepAnalytics API Token",
      "<token>");

  flags.defineFlag(
      "statsd_port",
      util::cli::FlagParser::T_INTEGER,
      false,
      "p",
      "8125",
      "StatsD listen port",
      "<port>");

  flags.defineFlag(
      "key_prefix",
      util::cli::FlagParser::T_STRING,
      false,
      NULL,
      "stats.",
      "StatsD metric key prefix",
      "<string>");

  flags.defineFlag(
      "loglevel",
      util::cli::FlagParser::T_STRING,
      false,
      NULL,
      "INFO",
      "loglevel",
      "<level>");

  flags.parseArgv(argc, argv);

  Logger::get()->setMinimumLogLevel(
      strToLogLevel(flags.getString("loglevel")));

  try {
    run(flags);
  } catch (const StandardException& e) {
    util::logError("dx-statsd-sink", "[FATAL ERROR] $0", e.what());
  }

  return 0;
}
