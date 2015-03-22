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
  input_feeds.emplace(
      "index_requests.feedserver01.nue01.production.fnrd.net",
      URI("http://s01.nue01.production.fnrd.net:7001/rpc"));
  input_feeds.emplace(
      "index_requests.feedserver02.nue01.production.fnrd.net",
      URI("http://s02.nue01.production.fnrd.net:7001/rpc"));

  /* resume from last offset */
  {
    auto txn = index_writer->dbTransaction();
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

    {
      auto txn = index_writer->dbTransaction();

      for (const auto& soff : stream_offsets) {
        txn->update(
            StringUtil::format("__indexfeed_offset~$0", soff.first),
            StringUtil::toString(soff.second));

        stream_offsets_str +=
            StringUtil::format("\n    offset[$0]=$1", soff.first, soff.second);
      }
    }

    fnord::logInfo(
        "cm.indexbuild",
        "IndexWriter comitting...$0",
        stream_offsets_str);

    index_writer->commit();

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

void importSSTable(RefPtr<IndexWriter> index_writer, const String& filename) {
  fnord::logInfo("cm.indexbuild", "Importing sstable: $0", filename);
  sstable::SSTableReader reader(File::openFile(filename, File::O_READ));

  if (reader.bodySize() == 0) {
    fnord::logCritical("cm.indexbuild", "unfinished table: $0", filename);
    return;
  }

  /* get sstable cursor */
  auto cursor = reader.getCursor();
  auto body_size = reader.bodySize();
  int row_idx = 0;

  /* status line */
  util::SimpleRateLimitedFn status_line(kMicrosPerSecond, [&] () {
    fnord::logInfo(
        "cm.indexbuild",
        "[$0%] Reading sstable... rows=$1",
        (size_t) ((cursor->position() / (double) body_size) * 100),
        row_idx);
  });

  /* read sstable rows */
  for (; cursor->valid(); ++row_idx) {
    status_line.runMaybe();

    auto val = cursor->getDataBuffer();

    Option<cm::IndexRequest> idx_req;
    try {
      idx_req = Some(json::fromJSON<cm::IndexRequest>(val));
    } catch (const Exception& e) {
      fnord::logWarning("cm.indexbuld", e, "invalid json: $0", val.toString());
    }

    if (!idx_req.isEmpty()) {
      index_writer->updateDocument(idx_req.get());
    }

    if (!cursor->next()) {
      break;
    }
  }

  status_line.runForce();
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
      fnord::cli::FlagParser::T_SWITCH,
      false,
      NULL,
      NULL,
      "rebuild fts index",
      "");

  flags.defineFlag(
      "docid",
      fnord::cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "docid",
      "<docid>");

  flags.defineFlag(
      "import_sstable",
      fnord::cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "import sstable",
      "<file>");

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
    auto docid = flags.getString("docid");

    if (docid == "all") {
      index_writer->rebuildFTS();
    } else {
      index_writer->rebuildFTS(cm::DocID { .docid = docid });
    }
  } else if (flags.isSet("import_sstable")) {
    auto sstable_filename = flags.getString("import_sstable");
    importSSTable(index_writer, sstable_filename);
  } else {
    buildIndexFromFeed(index_writer, flags);
  }

  index_writer->commit();
  fnord::logInfo("cm.indexbuild", "Exiting...");
  return 0;
}

