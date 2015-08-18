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
#include "stx/io/filerepository.h"
#include "stx/io/fileutil.h"
#include "stx/application.h"
#include "stx/logging.h"
#include "stx/random.h"
#include "stx/thread/eventloop.h"
#include "stx/thread/threadpool.h"
#include "stx/thread/queue.h"
#include "stx/wallclock.h"
#include "stx/cli/flagparser.h"
#include "fnord-logtable/TableRepository.h"
#include "fnord-logtable/LogTableTail.h"
#include "stx/protobuf/MessageEncoder.h"
#include "stx/protobuf/MessagePrinter.h"
#include "logjoin/LogJoinBackfill.h"
#include "IndexReader.h"

#include "common.h"
#include "schemas.h"

using namespace zbase;
using namespace stx;

stx::thread::EventLoop ev;
std::atomic<bool> cm_logjoin_shutdown;

void quit(int n) {
  cm_logjoin_shutdown = true;
}

struct BackfillData {
  Option<uint64_t> shop_id;
  Option<uint64_t> category1;
  Option<uint64_t> category2;
  Option<uint64_t> category3;
};

int main(int argc, const char** argv) {
  stx::Application::init();
  stx::Application::logToStderr();

  cm_logjoin_shutdown = false;
  struct sigaction sa;
  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_handler = quit;
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGQUIT, &sa, NULL);
  sigaction(SIGINT, &sa, NULL);

  stx::cli::FlagParser flags;

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
      stx::cli::FlagParser::T_STRING,
      false,
      NULL,
      "http://localhost:8000",
      "feedserver addr",
      "<addr>");

  flags.defineFlag(
      "batch_size",
      stx::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "2048",
      "batch_size",
      "<num>");

  flags.defineFlag(
      "worker_threads",
      stx::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "4",
      "threads",
      "<num>");

  flags.defineFlag(
      "upload_threads",
      stx::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "1",
      "threads",
      "<num>");

  flags.defineFlag(
      "no_dryrun",
      stx::cli::FlagParser::T_SWITCH,
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
     "pinfo_csv",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "file",
      "<file>");

  flags.defineFlag(
      "no_dryrun",
      stx::cli::FlagParser::T_SWITCH,
      false,
      NULL,
      NULL,
      "no dryrun",
      "<bool>");

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


  /* args */
  auto index_path = flags.getString("index");
  auto batch_size = flags.getInt("batch_size");
  auto datadir = flags.getString("datadir");
  auto dry_run = !flags.isSet("no_dryrun");
  if (!FileUtil::isDirectory(datadir)) {
    RAISEF(kIOError, "no such directory: $0", datadir);
  }

  Vector<URI> target_uris = {
    URI("http://nue03.prod.fnrd.net:7003/logtable/insert?table=joined_sessions-dawanda"),
    URI("http://nue03.prod.fnrd.net:7003/logtable/insert?table=joined_sessions-dawanda"),
    URI("http://nue02.prod.fnrd.net:7003/logtable/insert?table=joined_sessions-dawanda"),
    URI("http://nue02.prod.fnrd.net:7003/logtable/insert?table=joined_sessions-dawanda"),
    URI("http://nue02.prod.fnrd.net:7003/logtable/insert?table=joined_sessions-dawanda")
  };


  /* event loop, http */
  http::HTTPConnectionPool http(&ev);
  auto evloop_thread = std::thread([] {
    ev.run();
  });


  /* open table */
  auto schema = joinedSessionsSchema();
  auto table = logtable::TableReader::open(
      "dawanda_joined_sessions",
      flags.getString("replica"),
      datadir,
      schema);


  /* warmup cache */
  HashMap<String, BackfillData> cache;
  std::mutex cache_mutex;

  if (flags.isSet("pinfo_csv")) {
    io::MmappedFile mmap(File::openFile(flags.getString("pinfo_csv"), File::O_READ));

    size_t begin = 0;
    size_t end = mmap.size();
    while (begin < end) {
      size_t len = 0;
      for (; *mmap.structAt<char>(begin + len) != '\n' && (begin + len) < end; ++len);
      String line(mmap.structAt<char>(begin), len);
      begin += len + 1;

      if (len == 0) {
        continue;
      }

      auto cols = StringUtil::split(line, ",");
      if (cols.size() != 6) {
        continue;
      }

      BackfillData data;
      try { data.shop_id =  Some((uint64_t) std::stoul(cols[4])); } catch (...) {}
      try { data.category3 = Some((uint64_t) std::stoul(cols[1])); } catch (...) {}
      try { data.category2 = Some((uint64_t) std::stoul(cols[2])); } catch (...) {}
      try { data.category1 = Some((uint64_t) std::stoul(cols[3])); } catch (...) {}
      cache.emplace("p~" + cols[0], data);
    }
  }


  /* open index */
  auto index = zbase::IndexReader::openIndex(index_path);


  /* backfill fn */
  auto get_backfill_data = [&cache, &index, &cache_mutex] (const String& item_id) -> BackfillData {
    std::unique_lock<std::mutex> lk(cache_mutex);
    auto cached = cache.find(item_id);
    if (!(cached == cache.end())) {
      return cached->second;
    }
    lk.unlock();

    BackfillData data;

    DocID docid { .docid = item_id };
    auto shopid = index->docIndex()->getField(docid, "shop_id");
    if (!shopid.isEmpty()) {
      data.shop_id = Some((uint64_t) std::stoull(shopid.get()));
    }

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

    lk.lock();
    cache[item_id] = data;
    lk.unlock();
    return data;
  };

  auto queries_fid = schema.id("queries");
  auto queryitems_fid = schema.id("queries.items");
  auto qi_id_fid = schema.id("queries.items.item_id");
  auto qi_sid_fid = schema.id("queries.items.shop_id");
  auto qi_c1_fid = schema.id("queries.items.category1");
  auto qi_c2_fid = schema.id("queries.items.category2");
  auto qi_c3_fid = schema.id("queries.items.category3");
  auto backfill_fn = [
    &schema,
    &queries_fid,
    &queryitems_fid,
    &qi_id_fid,
    &qi_sid_fid,
    &qi_c1_fid,
    &qi_c2_fid,
    &qi_c3_fid,
    &get_backfill_data
  ] (msg::MessageObject* record) {
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
  };


  /* run backfill */
  zbase::LogJoinBackfill backfill(
      table,
      backfill_fn,
      "logjoin-backfill-state",
      dry_run,
      target_uris,
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

