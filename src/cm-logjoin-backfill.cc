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
#include <signal.h>
#include "fnord-base/io/filerepository.h"
#include "fnord-base/io/fileutil.h"
#include "fnord-base/application.h"
#include "fnord-base/logging.h"
#include "fnord-base/random.h"
#include "fnord-base/thread/eventloop.h"
#include "fnord-base/thread/threadpool.h"
#include "fnord-base/wallclock.h"
#include "fnord-base/cli/flagparser.h"
#include "fnord-eventdb/TableRepository.h"
#include "fnord-eventdb/LogTableTail.h"

#include "common.h"
#include "schemas.h"

using namespace cm;
using namespace fnord;

fnord::thread::EventLoop ev;
std::atomic<bool> cm_logjoin_shutdown;

int main(int argc, const char** argv) {
  fnord::Application::init();
  fnord::Application::logToStderr();

  cm_logjoin_shutdown = false;

  fnord::cli::FlagParser flags;

  flags.defineFlag(
      "datadir",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "datadir",
      "<path>");

  flags.defineFlag(
      "feedserver_addr",
      fnord::cli::FlagParser::T_STRING,
      false,
      NULL,
      "http://localhost:8000",
      "feedserver addr",
      "<addr>");

  flags.defineFlag(
      "batch_size",
      fnord::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "2048",
      "batch_size",
      "<num>");

  flags.defineFlag(
      "no_dryrun",
      fnord::cli::FlagParser::T_SWITCH,
      false,
      NULL,
      NULL,
      "no dryrun",
      "<bool>");

  flags.defineFlag(
      "replica",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "replica id",
      "<id>");

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

  /* args */
  auto batch_size = flags.getInt("batch_size");
  auto datadir = flags.getString("datadir");
  if (!FileUtil::isDirectory(datadir)) {
    RAISEF(kIOError, "no such directory: $0", datadir);
  }

  /* start event loop */
  auto evloop_thread = std::thread([] {
    ev.run();
  });

  /* set up http */
  http::HTTPConnectionPool http(&ev);

  /* open table */
  auto joined_sessions_schema = joinedSessionsSchema();
  auto table_reader = eventdb::TableReader::open(
      "dawanda_joined_sessions",
      flags.getString("replica"),
      datadir,
      joined_sessions_schema);

  eventdb::LogTableTail tail(table_reader);

  /* on record callback */
  auto on_record = [] (const msg::MessageObject& record) -> bool {
    fnord::iputs("on record...", 1);
    return true;
  };

  /* read table */
  fnord::logInfo(
      "cm.logjoin",
      "Starting LogJoin backfill with:\n    batch_size=$0",
      batch_size);

  bool running = true;
  while (running) {
    running = tail.fetchNext(on_record, batch_size);

    //fnord::logInfo("cm.logjoin", "LogJoin backfill comitting");

    if (cm_logjoin_shutdown.load()) {
      break;
    }
  };

  fnord::logInfo("cm.logjoin", "Draining LogJoin upload...");

  fnord::logInfo("cm.logjoin", "LogJoin backfill finished succesfully :)");

  ev.shutdown();
  evloop_thread.join();

  return 0;
}

