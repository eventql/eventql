/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <eventql/util/stdtypes.h>
#include <eventql/util/cli/flagparser.h>
#include <eventql/util/application.h>
#include <eventql/util/logging.h>
#include <eventql/util/random.h>
#include <eventql/util/wallclock.h>
#include <eventql/util/thread/eventloop.h>
#include <eventql/util/thread/threadpool.h>
#include <eventql/util/http/httpconnectionpool.h>
#include <eventql/dproc/Application.h>
#include <eventql/dproc/LocalScheduler.h>
#include <eventql/core/TSDBClient.h>
#include "eventql/FeedExportApp.h"

using namespace stx;
using namespace zbase;

stx::thread::EventLoop ev;

int main(int argc, const char** argv) {
  stx::Application::init();
  stx::Application::logToStderr();

  stx::cli::FlagParser flags;

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
      stx::cli::FlagParser::T_STRING,
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
  zbase::TSDBClient tsdb("http://nue03.prod.fnrd.net:7003/tsdb", &http);

  dproc::LocalScheduler sched(flags.getString("tempdir"), flags.getInt("threads"));
  sched.start();

  TSDBTableScanSpec params;
  params.set_table_name("joined_sessions.dawanda");
  params.set_partition_sha1("am9pbmVkX3Nlc3Npb25zLmRhd2FuZGEbgK2LqwU=");
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

