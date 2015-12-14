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
#include "stx/stats/statsd.h"
#include "stx/thread/FixedSizeThreadPool.h"
#include "stx/http/httpclient.h"
#include "stx/util/SimpleRateLimit.h"
#include "stx/protobuf/MessageSchema.h"

using namespace stx;

void run(const cli::FlagParser& flags) {
  String api_url = "http://api.zbase.io/api/v1";
  if (getenv("DX_DEVEL")) {
    api_url = "http://dev.zbase.io/api/v1";
  }

  auto api_token = flags.getString("api_token");
  auto statsd_port = flags.getInt("statsd_port");
  auto prefix = flags.getString("key_prefix");

  thread::EventLoop ev;
  thread::FixedSizeThreadPool tpool(4);
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
  stx::Application::init();
  stx::Application::logToStderr();

  stx::cli::FlagParser flags;

  flags.defineFlag(
      "api_token",
      stx::cli::FlagParser::T_STRING,
      true,
      "x",
      NULL,
      "DeepAnalytics API Token",
      "<token>");

  flags.defineFlag(
      "statsd_port",
      stx::cli::FlagParser::T_INTEGER,
      false,
      "p",
      "8125",
      "StatsD listen port",
      "<port>");

  flags.defineFlag(
      "key_prefix",
      stx::cli::FlagParser::T_STRING,
      false,
      NULL,
      "stats.",
      "StatsD metric key prefix",
      "<string>");

  flags.defineFlag(
      "loglevel",
      stx::cli::FlagParser::T_STRING,
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
    stx::logError("dx-statsd-sink", "[FATAL ERROR] $0", e.what());
  }

  return 0;
}
