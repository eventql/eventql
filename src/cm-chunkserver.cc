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
#include "fnord-base/thread/FixedSizeThreadPool.h"
#include "fnord-base/wallclock.h"
#include "fnord-base/VFS.h"
#include "fnord-rpc/ServerGroup.h"
#include "fnord-rpc/RPC.h"
#include "fnord-rpc/RPCClient.h"
#include "fnord-base/cli/flagparser.h"
#include "fnord-json/json.h"
#include "fnord-json/jsonrpc.h"
#include "fnord-http/httprouter.h"
#include "fnord-http/httpserver.h"
#include "fnord-http/VFSFileServlet.h"
#include "fnord-feeds/FeedService.h"
#include "fnord-feeds/RemoteFeedFactory.h"
#include "fnord-feeds/RemoteFeedReader.h"
#include "fnord-base/stats/statsdagent.h"
#include "fnord-sstable/SSTableServlet.h"
#include "fnord-logtable/LogTableServlet.h"
#include "fnord-logtable/TableRepository.h"
#include "fnord-logtable/TableJanitor.h"
#include "fnord-logtable/TableReplication.h"
#include "fnord-logtable/ArtifactReplication.h"
#include "fnord-logtable/ArtifactIndexReplication.h"
#include "fnord-logtable/NumericBoundsSummary.h"
#include "fnord-mdb/MDB.h"
#include "fnord-mdb/MDBUtil.h"
#include "fnord-tsdb/TSDBNode.h"
#include "fnord-tsdb/TSDBServlet.h"
#include "common.h"
#include "schemas.h"
#include "ModelReplication.h"

using namespace fnord;

std::atomic<bool> shutdown_sig;
fnord::thread::EventLoop ev;

void quit(int n) {
  shutdown_sig = true;
  fnord::logInfo("cm.chunkserver", "Shutting down...");
  // FIXPAUL: wait for http server stop...
  ev.shutdown();
}

int main(int argc, const char** argv) {
  fnord::Application::init();
  fnord::Application::logToStderr();

  /* shutdown hook */
  shutdown_sig = false;
  struct sigaction sa;
  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_handler = quit;
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGQUIT, &sa, NULL);
  sigaction(SIGINT, &sa, NULL);

  fnord::cli::FlagParser flags;

  flags.defineFlag(
      "http_port",
      fnord::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "8000",
      "Start the public http server on this port",
      "<port>");

  flags.defineFlag(
      "readonly",
      cli::FlagParser::T_SWITCH,
      false,
      NULL,
      NULL,
      "readonly",
      "readonly");

  flags.defineFlag(
      "replica",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "replica id",
      "<id>");

  flags.defineFlag(
      "datadir",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "datadir path",
      "<path>");

  flags.defineFlag(
      "replicate_from",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "url",
      "<url>");

  flags.defineFlag(
      "fsck",
      cli::FlagParser::T_SWITCH,
      false,
      NULL,
      NULL,
      "fsck",
      "fsck");

  flags.defineFlag(
      "repair",
      cli::FlagParser::T_SWITCH,
      false,
      NULL,
      NULL,
      "repair",
      "repair");

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
  auto dir = flags.getString("datadir");
  auto readonly = flags.isSet("readonly");
  auto replica = flags.getString("replica");
  auto repl_sources = flags.getStrings("replicate_from");

  Vector<URI> artifact_sources;
  for (const auto& rep : repl_sources) {
    artifact_sources.emplace_back(
        URI(StringUtil::format("http://$0:7005/", rep)));
  }

  /* start http server and worker pools */
  fnord::thread::ThreadPool tpool;
  fnord::thread::FixedSizeThreadPool wpool(8);
  fnord::thread::FixedSizeThreadPool repl_wpool(8);
  http::HTTPConnectionPool http(&ev);
  fnord::http::HTTPRouter http_router;
  fnord::http::HTTPServer http_server(&http_router, &ev);
  http_server.listen(flags.getInt("http_port"));
  wpool.start();
  repl_wpool.start();

  /* artifact replication */
  logtable::ArtifactReplication artifact_replication(
      &http,
      &repl_wpool,
      8);

  /* model replication */
  ModelReplication model_replication;

  if (!readonly) {
    model_replication.addArtifactIndexReplication(
        "termstats",
        dir,
        artifact_sources,
        &artifact_replication,
        &http);
  }

  /* logtable */
  logtable::TableRepository table_repo(
      dir,
      replica,
      readonly,
      &wpool);

  auto joined_sessions_schema = joinedSessionsSchema();
  table_repo.addTable("joined_sessions-dawanda", joined_sessions_schema);
  table_repo.addTable("index_feed-dawanda", indexChangeRequestSchema());
  Set<String> tbls  = { "joined_sessions-dawanda", "index_feed-dawanda" };

  logtable::TableReplication table_replication(&http);
  if (!readonly) {
    for (const auto& tbl : tbls) {
      auto table = table_repo.findTableWriter(tbl);

      if (StringUtil::beginsWith(tbl, "joined_sessions")) {
        table->addSummary([joined_sessions_schema] () {
          return new logtable::NumericBoundsSummaryBuilder(
              "search_queries.time-bounds",
              joined_sessions_schema.id("search_queries.time"));
        });
      }

      table->runConsistencyCheck(
          flags.isSet("fsck"),
          flags.isSet("repair"));

      for (const auto& rep : repl_sources) {
        table_replication.replicateTableFrom(
            table,
            URI(StringUtil::format("http://$0:7003/logtable", rep)));
      }

      if (artifact_sources.size() > 0) {
        artifact_replication.replicateArtifactsFrom(
            table->artifactIndex(),
            artifact_sources);
      }
    }
  }

  logtable::TableJanitor table_janitor(&table_repo);
  if (!readonly) {
    table_janitor.start();
    table_replication.start();
    artifact_replication.start();
    model_replication.start();
  }

  logtable::LogTableServlet logtable_servlet(&table_repo);
  http_router.addRouteByPrefixMatch("/logtable", &logtable_servlet, &tpool);


  tsdb::TSDBNode tsdb_node(replica, dir + "/tsdb");

  tsdb::StreamProperties config(new msg::MessageSchema(joinedSessionsSchema()));
  config.max_datafile_size= 1024 * 1024 * 512;
  config.compaction_interval = Duration(300 * kMicrosPerSecond);
  tsdb_node.configurePrefix("joined_sessions.", config);

  tsdb::TSDBServlet tsdb_servlet(&tsdb_node);
  http_router.addRouteByPrefixMatch("/tsdb", &tsdb_servlet, &tpool);

  tsdb_node.start();

  ev.run();

  tsdb_node.stop();

  if (!readonly) {
    table_janitor.stop();
    table_janitor.check();
    table_replication.stop();
    artifact_replication.stop();
    model_replication.stop();
  }

  fnord::logInfo("cm.chunkserver", "Exiting...");

  exit(0);
}

