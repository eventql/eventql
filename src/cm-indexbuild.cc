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
#include "fnord/io/filerepository.h"
#include "fnord/io/fileutil.h"
#include "fnord/application.h"
#include "fnord/logging.h"
#include "fnord/random.h"
#include "fnord/thread/eventloop.h"
#include "fnord/thread/threadpool.h"
#include "fnord/wallclock.h"
#include "fnord/util/SimpleRateLimit.h"
#include "fnord-rpc/ServerGroup.h"
#include "fnord-rpc/RPC.h"
#include "fnord-rpc/RPCClient.h"
#include "fnord/cli/flagparser.h"
#include "fnord/json/json.h"
#include "fnord/json/jsonrpc.h"
#include "fnord/http/httprouter.h"
#include "fnord/http/httpserver.h"
#include "fnord-feeds/FeedService.h"
#include "fnord-feeds/RemoteFeedFactory.h"
#include "fnord-feeds/RemoteFeedReader.h"
#include "fnord/stats/statsdagent.h"
#include "fnord-sstable/sstablereader.h"
#include "fnord-logtable/RemoteTableReader.h"
#include "fnord-logtable/LogTableTail.h"
#include "fnord/mdb/MDB.h"
#include "fnord/protobuf/MessagePrinter.h"
#include "CustomerNamespace.h"

#include "DocIndex.h"
#include "IndexChangeRequest.h"
#include "schemas.h"

using namespace cm;
using namespace fnord;

fnord::thread::EventLoop ev;

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
      fnord::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "feed source",
      "<addr>");

  flags.defineFlag(
      "statsd_addr",
      fnord::cli::FlagParser::T_STRING,
      false,
      NULL,
      "127.0.0.1:8192",
      "Statsd addr",
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
      "commit_interval",
      fnord::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "5",
      "commit_interval",
      "<num>");

  flags.defineFlag(
      "db_size",
      fnord::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "128",
      "max db size",
      "<MB>");

  flags.defineFlag(
      "reset_cursor",
      fnord::cli::FlagParser::T_SWITCH,
      false,
      NULL,
      NULL,
      "reset cursor",
      "<switch>");

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

  auto evloop_thread = std::thread([] {
    ev.run();
  });

  http::HTTPConnectionPool http(&ev);

  /* args */
  auto batch_size = flags.getInt("batch_size");
  uint64_t rate_limit_micros = flags.getInt("commit_interval") * kMicrosPerSecond;

  /* open index */
  fnord::logInfo(
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

    fnord::logInfo(
        "cm.indexbuild",
        "Resuming from cursor: $0",
        cursor.debugPrint());

    tail = RefPtr<logtable::LogTableTail>(
        new logtable::LogTableTail(table.get(), cursor));
  }

  /* start stats reporting */
  fnord::stats::StatsdAgent statsd_agent(
      fnord::InetAddr::resolve(flags.getString("statsd_addr")),
      10 * fnord::kMicrosPerSecond);

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
  DateTime last_run;
  for (;;) {
    auto t0 = WallClock::unixMicros();
    auto eof_reached = !tail->fetchNext(on_record, batch_size);

    auto cursor = tail->getCursor();
    util::BinaryMessageWriter cursor_buf;
    cursor.encode(&cursor_buf);

    fnord::logInfo(
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

  fnord::logInfo("cm.indexbuild", "Exiting...");
  return 0;
}

