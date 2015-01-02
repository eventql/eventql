/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <stdlib.h>
#include <unistd.h>
#include "fnord/base/application.h"
#include "fnord/base/random.h"
#include "fnord/comm/lbgroup.h"
#include "fnord/comm/rpc.h"
#include "fnord/cli/flagparser.h"
#include "fnord/comm/rpcchannel.h"
#include "fnord/io/filerepository.h"
#include "fnord/io/fileutil.h"
#include "fnord/json/json.h"
#include "fnord/json/jsonrpc.h"
#include "fnord/json/jsonrpchttpchannel.h"
#include "fnord/net/http/httprouter.h"
#include "fnord/net/http/httpserver.h"
#include "fnord/thread/eventloop.h"
#include "fnord/thread/threadpool.h"
#include "fnord/service/logstream/logstreamservice.h"
#include "fnord/service/logstream/feedfactory.h"
#include "customernamespace.h"
#include "tracker/logjoin.h"

int main(int argc, const char** argv) {
  fnord::Application::init();
  fnord::Application::logToStderr();

  fnord::cli::FlagParser flags;

  flags.defineFlag(
      "feedserver_jsonrpc_url",
      fnord::cli::FlagParser::T_STRING,
      true,
      NULL,
      "8080",
      "feedserver jsonrpc url",
      "<port>");

  flags.defineFlag(
      "cm_env",
      fnord::cli::FlagParser::T_STRING,
      true,
      NULL,
      "dev",
      "cm env",
      "<env>");

  flags.defineFlag(
      "no_dryrun",
      fnord::cli::FlagParser::T_SWITCH,
      false,
      NULL,
      NULL,
      "no dryrun",
      "<bool>");

  flags.parseArgv(argc, argv);

  /* start event loop */
  fnord::thread::EventLoop ev;

  auto evloop_thread = std::thread([&ev] {
    ev.run();
  });

  /* set up feedserver channel */
  fnord::comm::RoundRobinLBGroup feedserver_lbgroup;
  fnord::json::JSONRPCHTTPChannel feedserver_chan(
      &feedserver_lbgroup,
      &ev);

  feedserver_lbgroup.addServer(flags.getString("feedserver_jsonrpc_url"));
  fnord::logstream_service::LogStreamServiceFeedFactory feeds(&feedserver_chan);

  auto feed = feeds.getFeed("cm.tracker.log");
  if (flags.getString("cm_env") == "production") {
    feed->setOption("batch_size", "10000");
    feed->setOption("buffer_size", "100000");
  }

  auto dry_run = !flags.isSet("no_dryrun");
  auto start_offset = 0;

  fnord::log::Logger::get()->logf(
      fnord::log::kInfo,
      "[cm-logjoin] Starting cm-logjoin with dry_run=$0 start_offset=$1",
      dry_run,
      start_offset);

  cm::LogJoin logjoin(&feeds, dry_run);

  for (;;) {
    std::string logline;
    int n = 0;

    for (; feed->getNextEntry(&logline) && n < 10000; ++n) {
      try {
        logjoin.insertLogline(logline);
      } catch (const std::exception& e) {
        fnord::log::Logger::get()->logException(
            fnord::log::kInfo,
            "invalid log line",
            e);
      }
    }

    fnord::log::Logger::get()->logf(
        fnord::log::kInfo,
        "[cm-logjoin] stream_time=$0 active_sessions=$1 offset=$2",
        logjoin.streamTime(),
        logjoin.numSessions(),
        feed->offset());

    if (n == 0) {
      usleep(1000000);
    }
  }

  evloop_thread.join();
  return 0;
}

