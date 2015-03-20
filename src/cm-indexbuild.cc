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
#include "fnord-mdb/MDB.h"
#include "CustomerNamespace.h"
#include "FeatureSchema.h"
#include "FeatureIndexWriter.h"
#include "IndexRequest.h"
#include "IndexWriter.h"

using namespace cm;
using namespace fnord;

std::atomic<bool> cm_indexbuild_shutdown;

/* stats */

void quit(int n) {
  cm_indexbuild_shutdown = true;
}

void buildIndexFromFeed(
    RefPtr<IndexWriter> index_writer,
    const cli::FlagParser& flags) {
  size_t batch_size = flags.getInt("batch_size");
  size_t buffer_size = flags.getInt("buffer_size");
  size_t db_commit_size = flags.getInt("db_commit_size");
  size_t db_commit_interval = flags.getInt("db_commit_interval");

  /* start event loop */
  fnord::thread::EventLoop ev;

  auto evloop_thread = std::thread([&ev] {
    ev.run();
  });

  /* set up rpc client */
  HTTPRPCClient rpc_client(&ev);

  feeds::RemoteFeedReader feed_reader(&rpc_client);
  feed_reader.setMaxSpread(3600 * 24 * 30 * kMicrosPerSecond);

  HashMap<String, URI> input_feeds;
  auto cmcustomer = "dawanda";
  input_feeds.emplace(
      StringUtil::format(
          "$0.index_requests.feedserver01.nue01.production.fnrd.net",
          cmcustomer),
      URI("http://s01.nue01.production.fnrd.net:7001/rpc"));
  input_feeds.emplace(
      StringUtil::format(
          "$0.index_requests.feedserver02.nue01.production.fnrd.net",
          cmcustomer),
      URI("http://s02.nue01.production.fnrd.net:7001/rpc"));

  /* resume from last offset */
  auto txn = index_writer->featureDB()->startTransaction(true);
  try {
    for (const auto& input_feed : input_feeds) {
      uint64_t offset = 0;

      auto last_offset = txn->get(
          StringUtil::format("__indexfeed_offset~$0", input_feed.first));

      if (!last_offset.isEmpty()) {
        offset = std::stoul(last_offset.get().toString());
      }

      fnord::logInfo(
          "cm.indexbuild",
          "Adding source feed:\n    feed=$0\n    url=$1\n    offset: $2",
          input_feed.first,
          input_feed.second.toString(),
          offset);

      feed_reader.addSourceFeed(
          input_feed.second,
          input_feed.first,
          offset,
          batch_size,
          buffer_size);
    }

    txn->abort();
  } catch (...) {
    txn->abort();
    throw;
  }

  fnord::logInfo("cm.indexbuild", "Resuming IndexWriter...");

  DateTime last_iter;
  uint64_t rate_limit_micros = db_commit_interval * kMicrosPerSecond;

  for (;;) {
    last_iter = WallClock::now();
    feed_reader.fillBuffers();
    int i = 0;
    for (; i < db_commit_size; ++i) {
      auto entry = feed_reader.fetchNextEntry();

      if (entry.isEmpty()) {
        break;
      }

      cm::IndexRequest index_req;
      const auto& entry_str = entry.get().data;
      try {
        index_req = json::fromJSON<cm::IndexRequest>(entry_str);
      } catch (const std::exception& e) {
        fnord::logError("cm.indexbuild", e, "invalid json: $0", entry_str);
        continue;
      }

      fnord::logTrace("cm.indexbuild", "Indexing: $0", entry.get().data);
      index_writer->updateDocument(index_req);
    }

    auto stream_offsets = feed_reader.streamOffsets();
    String stream_offsets_str;

    index_writer->commit();

    auto txn = index_writer->featureDB()->startTransaction();

    for (const auto& soff : stream_offsets) {
      txn->update(
          StringUtil::format("__indexfeed_offset~$0", soff.first),
          StringUtil::toString(soff.second));

      stream_offsets_str +=
          StringUtil::format("\n    offset[$0]=$1", soff.first, soff.second);
    }

    fnord::logInfo(
        "cm.indexbuild",
        "IndexWriter comitting...$0",
        stream_offsets_str);

    txn->commit();

    if (cm_indexbuild_shutdown.load() == true) {
      break;
    }

    auto etime = WallClock::now().unixMicros() - last_iter.unixMicros();
    if (i < 1 && etime < rate_limit_micros) {
      usleep(rate_limit_micros - etime);
    }
  }

  ev.shutdown();
  evloop_thread.join();
}

int main(int argc, const char** argv) {
  Application::init();
  Application::logToStderr();

  /* shutdown hook */
  cm_indexbuild_shutdown = false;
  struct sigaction sa;
  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_handler = quit;
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGQUIT, &sa, NULL);
  sigaction(SIGINT, &sa, NULL);

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
      "conf",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      "./conf",
      "conf dir",
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
      "buffer_size",
      fnord::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "8192",
      "buffer_size",
      "<num>");

  flags.defineFlag(
      "db_commit_size",
      fnord::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "1024",
      "db_commit_size",
      "<num>");

  flags.defineFlag(
      "db_commit_interval",
      fnord::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "5",
      "db_commit_interval",
      "<num>");

  flags.defineFlag(
      "dbsize",
      fnord::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "128",
      "max sessiondb size",
      "<MB>");

  flags.defineFlag(
      "rebuild_fts",
      fnord::cli::FlagParser::T_STRING,
      false,
      NULL,
      "all",
      "rebuild fts index",
      "");

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

  /* open index */
  fnord::logInfo(
      "cm.indexbuild",
      "Opening index at $0",
      flags.getString("index"));

  auto index_writer = cm::IndexWriter::openIndex(
      flags.getString("index"),
      flags.getString("conf"));

  index_writer->exportStats("/cm-indexbuild/global");

  if (flags.isSet("rebuild_fts")) {
    auto docid = flags.getString("rebuild_fts");

    if (docid == "all") {
      index_writer->rebuildFTS();
    } else {
      index_writer->rebuildFTS(cm::DocID { .docid = docid });
    }
  } else {
    buildIndexFromFeed(index_writer, flags);
  }

  index_writer->commit();
  fnord::logInfo("cm.indexbuild", "Exiting...");
  return 0;
}













/* open lucene index */
  /*
  auto index_path = FileUtil::joinPaths(
      cmdata_path,
      StringUtil::format("index/$0/fts", cmcustomer));
  FileUtil::mkdir_p(index_path);

  */
  // index document
/*
*/


  //index_writer->commit();
  //index_writer->close();

  // simple search
  /*
  auto index_reader = fts::IndexReader::open(
      fts::FSDirectory::open(StringUtil::convertUTF8To16(index_path)),
      true);

  auto searcher = fts::newLucene<fts::IndexSearcher>(index_reader);

  auto analyzer = fts::newLucene<fts::StandardAnalyzer>(
      fts::LuceneVersion::LUCENE_CURRENT);

  auto query_parser = fts::newLucene<fts::QueryParser>(
      fts::LuceneVersion::LUCENE_CURRENT,
      L"keywords",
      analyzer);

  auto collector = fts::TopScoreDocCollector::create(
      500,
      false);

  auto query = query_parser->parse(L"fnordy");

  searcher->search(query, collector);
  fnord::iputs("found $0 documents", collector->getTotalHits());
  */


