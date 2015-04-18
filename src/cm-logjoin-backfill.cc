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
#include "fnord-msg/MessagePrinter.h"
#include "logjoin/LogJoinBackfill.h"
#include "IndexReader.h"

#include "common.h"
#include "schemas.h"

using namespace cm;
using namespace fnord;

fnord::thread::EventLoop ev;
std::atomic<bool> cm_logjoin_shutdown;

void quit(int n) {
  cm_logjoin_shutdown = true;
}

struct BackfillData {
  Option<String> shop_name;
  Option<String> shop_id;
  Option<uint64_t> category1;
  Option<uint64_t> category2;
  Option<uint64_t> category3;
};

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
      "index",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "index directory",
      "<path>");

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
  auto index_path = flags.getString("index");
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


  /* open index */
  auto index = cm::IndexReader::openIndex(index_path);


  /* open table */
  auto schema = joinedSessionsSchema();
  auto table = eventdb::TableReader::open(
      "dawanda_joined_sessions",
      flags.getString("replica"),
      datadir,
      schema);


  auto queries_fid = schema.id("queries");
  auto queryitems_fid = schema.id("queries.items");
  auto qi_id_fid = schema.id("queries.items.item_id");
  auto qi_sid_fid = schema.id("queries.items.shop_id");
  auto qi_sname_fid = schema.id("queries.items.shop_name");
  auto qi_c1_fid = schema.id("queries.items.category1");
  auto qi_c2_fid = schema.id("queries.items.category2");
  auto qi_c3_fid = schema.id("queries.items.category3");

  HashMap<String, BackfillData> cache;

  /* backfill fn */
  auto get_backfill_data = [&cache, &index] (const String& item_id) -> BackfillData {
    auto cached = cache.find(item_id);
    if (!(cached == cache.end())) {
      return cached->second;
    }

    BackfillData data;

    DocID docid { .docid = item_id };
    data.shop_id = index->docIndex()->getField(docid, "shop_id");
    data.shop_name = index->docIndex()->getField(docid, "shop_name");

    auto category1 = index->docIndex()->getField(docid, "category1");
    if (!category1.isEmpty()) {
      data.category1 = Some((uint64_t) std::stoull(category1.get()));
    }

    auto category2 = index->docIndex()->getField(docid, "category2");
    if (!category2.isEmpty()) {
      data.category2 = Some((uint64_t) std::stoull(category2.get()));
    }

    auto category3 = index->docIndex()->getField(docid, "category3");
    if (!category3.isEmpty()) {
      data.category3 = Some((uint64_t) std::stoull(category3.get()));
    }

    cache[item_id] = data;
    return data;
  };

  auto backfill_fn = [
    &schema,
    &queries_fid,
    &queryitems_fid,
    &qi_id_fid,
    &qi_sid_fid,
    &qi_sname_fid,
    &qi_c1_fid,
    &qi_c2_fid,
    &qi_c3_fid,
    &get_backfill_data
  ] (msg::MessageObject* record) {
    auto msg = msg::MessagePrinter::print(*record, schema);

    for (auto& q : record->asObject()) {
      if (q.id != queries_fid) {
        continue;
      }

      for (auto& qi : q.asObject()) {
        if (qi.id != queryitems_fid) {
          continue;
        }

        String item_id;
        for (auto cur = qi.asObject().begin(); cur != qi.asObject().end(); ) {
          auto id = cur->id;

          if (id == qi_id_fid) {
            item_id = cur->asString();
            ++cur;
          } else if (
              id == qi_sid_fid ||
              id == qi_sname_fid ||
              id == qi_c1_fid ||
              id == qi_c2_fid ||
              id == qi_c3_fid) {
            cur = qi.asObject().erase(cur);
          } else {
            ++cur;
          }
        }

        auto bdata = get_backfill_data(item_id);

        if (!bdata.shop_id.isEmpty()) {
          qi.addChild(qi_sid_fid, bdata.shop_id.get());
        }

        if (!bdata.shop_name.isEmpty()) {
          qi.addChild(qi_sname_fid, bdata.shop_name.get());
        }

        if (!bdata.category1.isEmpty()) {
          qi.addChild(qi_c1_fid, bdata.category1.get());
        }

        if (!bdata.category2.isEmpty()) {
          qi.addChild(qi_c2_fid, bdata.category2.get());
        }

        if (!bdata.category3.isEmpty()) {
          qi.addChild(qi_c3_fid, bdata.category3.get());
        }
      }
    }

    fnord::iputs("backfill: $0", msg);
  };


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

