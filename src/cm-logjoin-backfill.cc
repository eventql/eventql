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
#include "fnord-base/thread/queue.h"
#include "fnord-base/wallclock.h"
#include "fnord-base/cli/flagparser.h"
#include "fnord-eventdb/TableRepository.h"
#include "fnord-eventdb/LogTableTail.h"
#include "fnord-msg/MessageEncoder.h"
#include "logjoin/LogJoinBackfill.h"

#include "common.h"
#include "schemas.h"

using namespace cm;
using namespace fnord;

fnord::thread::EventLoop ev;
std::atomic<bool> cm_logjoin_shutdown;

void quit(int n) {
  cm_logjoin_shutdown = true;
}

int main(int argc, const char** argv) {
  fnord::Application::init();
  fnord::Application::logToStderr();

  cm_logjoin_shutdown = false;
  struct sigaction sa;
  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_handler = quit;
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGQUIT, &sa, NULL);
  sigaction(SIGINT, &sa, NULL);

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
      "worker_threads",
      fnord::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "4",
      "threads",
      "<num>");

  flags.defineFlag(
      "upload_threads",
      fnord::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "1",
      "threads",
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
      "no_dryrun",
      fnord::cli::FlagParser::T_SWITCH,
      false,
      NULL,
      NULL,
      "no dryrun",
      "<bool>");

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
  auto dry_run = !flags.isSet("no_dryrun");
  if (!FileUtil::isDirectory(datadir)) {
    RAISEF(kIOError, "no such directory: $0", datadir);
  }

  URI target_uri("http://localhost:8000/eventdb/insert?table=joined_sessions-dawanda");


  /* event loop, http */
  http::HTTPConnectionPool http(&ev);
  auto evloop_thread = std::thread([] {
    ev.run();
  });


  /* backfill fn */
  auto backfill_fn = [] (msg::MessageObject* record) {
    //fnord::iputs("backfill...", 1);
  };


  /* open table */
  auto schema = joinedSessionsSchema();
  auto table = eventdb::TableReader::open(
      "dawanda_joined_sessions",
      flags.getString("replica"),
      datadir,
      schema);


  /* run backfill */
  cm::LogJoinBackfill backfill(
      table,
      backfill_fn,
      "/tmp/logjoin-backfill-state",
      dry_run,
      target_uri,
      &http);

  backfill.start();

  while (backfill.process(batch_size)) {
    if (cm_logjoin_shutdown.load()) {
      break;
    }
  }

  backfill.shutdown();
  ev.shutdown();
  evloop_thread.join();

  return 0;
}

