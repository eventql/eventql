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
#include "fnord-base/util/SimpleRateLimit.h"
#include "fnord-rpc/ServerGroup.h"
#include "fnord-rpc/RPC.h"
#include "fnord-rpc/RPCClient.h"
#include "fnord-base/cli/flagparser.h"
#include "fnord-json/json.h"
#include "fnord-json/jsonrpc.h"
#include "fnord-http/httprouter.h"
#include "fnord-http/httpserver.h"
#include "fnord-feeds/FeedService.h"
#include "fnord-feeds/RemoteFeedFactory.h"
#include "fnord-feeds/RemoteFeedReader.h"
#include "fnord-base/stats/statsdagent.h"
#include "fnord-sstable/sstablereader.h"
#include "fnord-logtable/RemoteTableReader.h"
#include "fnord-logtable/LogTableTail.h"
#include "fnord-mdb/MDB.h"
#include "fnord-msg/MessagePrinter.h"
#include "CustomerNamespace.h"
#include "FeatureSchema.h"
#include "FeatureIndexWriter.h"
#include "IndexChangeRequest.h"
#include "IndexWriter.h"
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
      "db_size",
      fnord::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "128",
      "max db size",
      "<MB>");

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

  /* open index */
  fnord::logInfo(
      "cm.indexbuild",
      "Opening index at $0",
      flags.getString("index"));

  FeatureIndexWriter index(
      flags.getString("index"),
      "documents-dawanda",
      false);

  /* open logtable */
  RefPtr<logtable::RemoteTableReader> table(new logtable::RemoteTableReader(
      "index_feed-dawanda",
      indexChangeRequestSchema(),
      URI(flags.getString("fetch_from")),
      &http));

  /* open logtable tail at last cursor */
  RefPtr<logtable::LogTableTail> tail(nullptr);
  auto last_cursor = index.getCursor();
  if (last_cursor.isEmpty()) {
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
      fnord::net::InetAddr::resolve(flags.getString("statsd_addr")),
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
  auto batch_size = flags.getInt("batch_size");
  for (;;) {
    tail->fetchNext(on_record, batch_size);

    auto cursor = tail->getCursor();
    util::BinaryMessageWriter cursor_buf;
    cursor.encode(&cursor_buf);

    fnord::logInfo(
        "cm.indexbuild",
        "Comitting with cursor: $0",
        cursor.debugPrint());

    index.saveCursor(cursor_buf.data(), cursor_buf.size());
    index.commit();
  }

  /* sthudown */
  ev.shutdown();
  evloop_thread.join();

  fnord::logInfo("cm.indexbuild", "Exiting...");
  return 0;
}

