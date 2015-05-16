/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <fnord-base/stdtypes.h>
#include <fnord-base/cli/flagparser.h>
#include <fnord-base/application.h>
#include <fnord-base/logging.h>
#include <fnord-base/random.h>
#include <fnord-base/thread/eventloop.h>
#include <fnord-base/thread/threadpool.h>
#include <fnord-http/httpconnectionpool.h>
#include <fnord-dproc/Application.h>
#include <fnord-dproc/LocalScheduler.h>
#include <fnord-tsdb/TSDBClient.h>
#include "ItemBoostScanlet.h"

using namespace fnord;
using namespace cm;

fnord::thread::EventLoop ev;

int main(int argc, const char** argv) {
  fnord::Application::init();
  fnord::Application::logToStderr();

  fnord::cli::FlagParser flags;

  flags.defineFlag(
      "tempdir",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "artifact directory",
      "<path>");

  flags.defineFlag(
      "loglevel",
      fnord::cli::FlagParser::T_STRING,
      false,
      NULL,
      "INFO",
      "loglevel",
      "<level>");

  flags.parseArgv(argc, argv);

  Logger::get()->setMinimumLogLevel(
      strToLogLevel(flags.getString("loglevel")));

  /* start event loop */
  auto evloop_thread = std::thread([] {
    ev.run();
  });

  http::HTTPConnectionPool http(&ev);
  tsdb::TSDBClient tsdb("http://nue03.prod.fnrd.net:7003/tsdb", &http);

  dproc::Application app("cm.itemboost");
  DistAnalyticsTableScan<ItemBoostScanlet>::registerWithApp(&app, &tsdb);

  dproc::LocalScheduler sched(flags.getString("tempdir"));
  sched.start();

  auto taskspec = DistAnalyticsTableScan<ItemBoostScanlet>::getTaskSpec(
      "dawanda",
      WallClock::unixMicros() - 30 * kMicrosPerDay,
      WallClock::unixMicros() - 6 * kMicrosPerHour,
      ItemBoostParams {});

  auto res = sched.run(&app, taskspec);

  fnord::iputs("exit...", 1);
  sched.stop();
  ev.shutdown();
  evloop_thread.join();
  exit(0);
}

