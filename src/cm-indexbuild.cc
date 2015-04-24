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

  /* start stats reporting */
  fnord::stats::StatsdAgent statsd_agent(
      fnord::net::InetAddr::resolve(flags.getString("statsd_addr")),
      10 * fnord::kMicrosPerSecond);

  statsd_agent.start();

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

  logtable::RemoteTableReader table(
      "index_feed-dawanda",
      indexChangeRequestSchema(),
      URI("http://nue03.prod.fnrd.net:7009/logtable"),
      &http);

  auto schema = indexChangeRequestSchema();
  auto on_record = [&schema, &index] (const msg::MessageObject& rec) -> bool {
    //auto customer = rec.getString(schema.id("customer"));

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

  table.fetchRecords("nue03", 1, 10, on_record);
  index.commit(true);

  ev.shutdown();
  evloop_thread.join();

  fnord::logInfo("cm.indexbuild", "Exiting...");
  return 0;
}

