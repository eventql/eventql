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
#include <fnord-base/wallclock.h>
#include <fnord-base/thread/eventloop.h>
#include <fnord-base/thread/threadpool.h>
#include <fnord-http/httpconnectionpool.h>
#include <fnord-dproc/Application.h>
#include <fnord-dproc/LocalScheduler.h>
#include <fnord-tsdb/TSDBClient.h>
#include "analytics/FeedExportApp.h"

using namespace fnord;
using namespace cm;

fnord::thread::EventLoop ev;

int main(int argc, const char** argv) {
  fnord::Application::init();
  fnord::Application::logToStderr();

  fnord::cli::FlagParser flags;

  flags.defineFlag(
      "output",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "output file",
      "<path>");

  flags.defineFlag(
      "tempdir",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "artifact directory",
      "<path>");

  flags.defineFlag(
      "threads",
      cli::FlagParser::T_INTEGER,
      true,
      NULL,
      "8",
      "nthreads",
      "<num>");

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

  dproc::LocalScheduler sched(flags.getString("tempdir"), flags.getInt("threads"));
  sched.start();

  TSDBTableScanParams params;
  params.set_stream_key("joined_sessions.dawanda");
  params.set_partition_key("am9pbmVkX3Nlc3Npb25zLmRhd2FuZGEbgK2LqwU=");
  params.set_sample_modulo(32);
  params.set_sample_index(1);

  FeedExportApp app(&tsdb);
  auto res = sched.run(&app, "ECommerceSearchQueriesFeed", *msg::encode(params));

  auto output_file = File::openFile(
      flags.getString("output"),
      File::O_CREATEOROPEN | File::O_WRITE | File::O_TRUNCATE);

  output_file.write(res->data(), res->size());

  sched.stop();
  ev.shutdown();
  evloop_thread.join();
  exit(0);
}

