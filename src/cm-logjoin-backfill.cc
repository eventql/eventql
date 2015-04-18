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

#include "common.h"
#include "schemas.h"

using namespace cm;
using namespace fnord;

fnord::thread::EventLoop ev;
std::atomic<bool> cm_logjoin_shutdown;
std::atomic<bool> cm_logjoin_worker_shutdown;

void quit(int n) {
  cm_logjoin_shutdown = true;
}

void backfillRecord(msg::MessageObject* record) {
  //fnord::iputs("backfill $0", record);
}

void runWorker(
     thread::Queue<std::shared_ptr<msg::MessageObject>>* inputq,
     thread::Queue<Buffer>* uploadq,
     const msg::MessageSchema& schema,
     bool dry_run) {
  for (int i = 0; i < 8192; ++i) {
    auto rec = inputq->poll();
    if (rec.isEmpty()) {
      return;
    }

    try {
      backfillRecord(rec.get().get());
    } catch (const Exception& e) {
      fnord::logError("cm.logjoin", e, "backfill error");
    }

    if (!dry_run) {
      Buffer msg_buf;
      msg::MessageEncoder::encode(*rec.get(), schema, &msg_buf);

      uploadq->insert(msg_buf, true);
    }
  }
}

void runUpload(
    thread::Queue<Buffer>* uploadq,
    const URI& uri,
    http::HTTPConnectionPool* http) {
  for (int i = 0; i < 8192; ++i) {
    auto rec = uploadq->poll();
    if (rec.isEmpty()) {
      return;
    }

    for (auto delay = kMicrosPerSecond / 10;
        true;
        usleep((delay = std::min(delay * 2, kMicrosPerSecond * 30)))) {
      try {
        http::HTTPRequest req(http::HTTPMessage::M_POST, uri.pathAndQuery());
        req.addHeader("Host", uri.hostAndPort());
        req.addHeader("Content-Type", "application/fnord-msg");
        req.addBody(rec.get());

        auto res = http->executeRequest(req);
        res.wait();

        const auto& r = res.get();
        if (r.statusCode() != 201) {
          RAISEF(kRuntimeError, "received non-201 response: $0", r.body().toString());
        }

        break;
      } catch (const Exception& e) {
        fnord::logError("cm.logjoin", e, "upload error");
      }
    }
  }
}

int main(int argc, const char** argv) {
  fnord::Application::init();
  fnord::Application::logToStderr();

  cm_logjoin_shutdown = false;
  cm_logjoin_worker_shutdown = false;
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

  thread::Queue<std::shared_ptr<msg::MessageObject>> input_queue(64000);
  thread::Queue<Buffer> upload_queue(64000);

  Vector<std::thread> threads;

  /* start workers */
  for (int i = flags.getInt("worker_threads"); i > 0; --i) {
    threads.emplace_back([&input_queue, &upload_queue, &joined_sessions_schema, dry_run] {
      while (input_queue.length() > 0 || !cm_logjoin_worker_shutdown.load()) {
        runWorker(&input_queue, &upload_queue, joined_sessions_schema, dry_run);
        usleep(10000);
      }
    });
  }

  for (int i = flags.getInt("upload_threads"); i > 0; --i) {
    threads.emplace_back([&upload_queue, &http, &target_uri] {
      while (upload_queue.length() > 0 || !cm_logjoin_worker_shutdown.load()) {
        runUpload(&upload_queue, target_uri, &http);
        usleep(10000);
      }
    });
  }

  /* on record callback */
  size_t num_records = 0;
  auto on_record = [&num_records, &input_queue] (
      const msg::MessageObject& record) -> bool {
    input_queue.insert(
      std::shared_ptr<msg::MessageObject>(
          new msg::MessageObject(record)), true);
    ++num_records;
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

    fnord::logInfo(
        "cm.logjoin",
        "LogJoin backfill comitting\n    num_records=$0\n    " \
        "inputq=$1\n    uploadq=$2",
        num_records,
        input_queue.length(),
        upload_queue.length());

    if (cm_logjoin_shutdown.load()) {
      break;
    }
  };

  while (input_queue.length() > 0 || upload_queue.length() > 0) {
    fnord::logInfo(
        "cm.logjoin",
        "Waiting for $0 jobs to finish...",
        input_queue.length() + upload_queue.length());

    usleep(1000000);
  }

  cm_logjoin_worker_shutdown = true;

  fnord::logInfo("cm.logjoin", "Waiting for jobs to finish...");
  for (auto& t : threads) {
    t.join();
  }

  fnord::logInfo("cm.logjoin", "LogJoin backfill finished succesfully :)");

  ev.shutdown();
  evloop_thread.join();

  return 0;
}

