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
#include "stx/wallclock.h"
#include "stx/util/SimpleRateLimit.h"
#include "stx/rpc/ServerGroup.h"
#include "stx/rpc/RPC.h"
#include "stx/rpc/RPCClient.h"
#include "stx/cli/flagparser.h"
#include "stx/json/json.h"
#include "stx/json/jsonrpc.h"
#include "stx/http/httprouter.h"
#include "stx/http/httpserver.h"
#include "brokerd/FeedService.h"
#include "brokerd/RemoteFeedFactory.h"
#include "brokerd/RemoteFeedReader.h"
#include "stx/stats/statsdagent.h"
#include "sstable/sstablereader.h"
#include "fnord-logtable/RemoteTableReader.h"
#include "fnord-logtable/LogTableTail.h"
#include "zbase/util/mdb/MDB.h"
#include "stx/protobuf/MessagePrinter.h"
#include "CustomerNamespace.h"

#include "DocIndex.h"
#include "IndexChangeRequest.h"
#include "schemas.h"

using namespace zbase;
using namespace stx;

stx::thread::EventLoop ev;

int main(int argc, const char** argv) {
  Application::init();
  Application::logToStderr();

  cli::FlagParser flags;

  flags.defineFlag(
      "index",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "index dir",
      "<path>");

  flags.defineFlag(
      "fetch_from",
      stx::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "feed source",
      "<addr>");

  flags.defineFlag(
      "statsd_addr",
      stx::cli::FlagParser::T_STRING,
      false,
      NULL,
      "127.0.0.1:8192",
      "Statsd addr",
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
      "commit_interval",
      stx::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "5",
      "commit_interval",
      "<num>");

  flags.defineFlag(
      "db_size",
      stx::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "128",
      "max db size",
      "<MB>");

  flags.defineFlag(
      "reset_cursor",
      stx::cli::FlagParser::T_SWITCH,
      false,
      NULL,
      NULL,
      "reset cursor",
      "<switch>");

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

  auto evloop_thread = std::thread([] {
    ev.run();
  });

  http::HTTPConnectionPool http(&ev);

  /* args */
  auto batch_size = flags.getInt("batch_size");
  uint64_t rate_limit_micros = flags.getInt("commit_interval") * kMicrosPerSecond;

  /* open index */
  stx::logInfo(
      "cm.indexbuild",
      "Opening index at $0",
      flags.getString("index"));

  DocIndex index(
      flags.getString("index"),
      "documents-dawanda",
      false);

  index.commit();
  exit(0);


  /* open logtable */
  RefPtr<logtable::RemoteTableReader> table(new logtable::RemoteTableReader(
      "index_feed-dawanda",
      indexChangeRequestSchema(),
      URI(flags.getString("fetch_from")),
      &http));

  /* open logtable tail at last cursor */
  RefPtr<logtable::LogTableTail> tail(nullptr);
  auto last_cursor = index.getCursor();
  if (last_cursor.isEmpty() || flags.isSet("reset_cursor")) {
    tail = RefPtr<logtable::LogTableTail>(
        new logtable::LogTableTail(table.get()));
  } else {
    util::BinaryMessageReader reader(
        last_cursor.get().data(),
        last_cursor.get().size());

    logtable::LogTableTailCursor cursor;
    cursor.decode(&reader);

    stx::logInfo(
        "cm.indexbuild",
        "Resuming from cursor: $0",
        cursor.debugPrint());

    tail = RefPtr<logtable::LogTableTail>(
        new logtable::LogTableTail(table.get(), cursor));
  }

  /* start stats reporting */
  stx::stats::StatsdAgent statsd_agent(
      stx::InetAddr::resolve(flags.getString("statsd_addr")),
      10 * stx::kMicrosPerSecond);

  statsd_agent.start();

  /* on record callback fn */
  auto schema = indexChangeRequestSchema();
  auto on_record = [&schema, &index] (const msg::MessageObject& rec) -> bool {
    Vector<Pair<String, String>> attrs;
    for (const auto& attr : rec.getObjects(schema.id("attributes"))) {
      auto key = attr->getString(schema.id("attributes.key"));
      auto value = attr->getString(schema.id("attributes.value"));

      if (isIndexAttributeWhitelisted(key)) {
        attrs.emplace_back(key, value);
      }
    }

    DocID docid { .docid = rec.getString(schema.id("docid")) };
    index.updateDocument(docid, attrs);

    return true;
  };

  /* fetch from logtable in batches */
  UnixTime last_run;
  for (;;) {
    auto t0 = WallClock::unixMicros();
    auto eof_reached = !tail->fetchNext(on_record, batch_size);

    auto cursor = tail->getCursor();
    util::BinaryMessageWriter cursor_buf;
    cursor.encode(&cursor_buf);

    stx::logInfo(
        "cm.indexbuild",
        "Comitting with cursor: $0",
        cursor.debugPrint());

    index.saveCursor(cursor_buf.data(), cursor_buf.size());
    index.commit();

    auto t1 = WallClock::unixMicros();
    if (eof_reached && t1 - t0 < rate_limit_micros) {
      usleep(rate_limit_micros - (t1 - t0));
    }
  }

  /* sthudown */
  ev.shutdown();
  evloop_thread.join();

  stx::logInfo("cm.indexbuild", "Exiting...");
  return 0;
}

