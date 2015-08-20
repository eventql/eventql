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
#include <sys/time.h>
#include <sys/resource.h>
#include "stx/io/filerepository.h"
#include "stx/io/fileutil.h"
#include "stx/application.h"
#include "stx/logging.h"
#include "stx/random.h"
#include "stx/thread/eventloop.h"
#include "stx/thread/threadpool.h"
#include "stx/thread/FixedSizeThreadPool.h"
#include "stx/wallclock.h"
#include "stx/VFS.h"
#include "stx/rpc/ServerGroup.h"
#include "stx/rpc/RPC.h"
#include "stx/rpc/RPCClient.h"
#include "stx/cli/flagparser.h"
#include "stx/json/json.h"
#include "stx/json/jsonrpc.h"
#include "stx/http/httprouter.h"
#include "stx/http/httpserver.h"
#include "stx/http/VFSFileServlet.h"
#include "brokerd/FeedService.h"
#include "brokerd/RemoteFeedFactory.h"
#include "brokerd/RemoteFeedReader.h"
#include "zbase/dproc/LocalScheduler.h"
#include "zbase/dproc/DispatchService.h"
#include "stx/stats/statsdagent.h"
#include "sstable/SSTableServlet.h"
#include "zbase/util/mdb/MDB.h"
#include "zbase/util/mdb/MDBUtil.h"
#include "zbase/AnalyticsServlet.h"
#include "zbase/ReportFactory.h"
#include "zbase/AnalyticsApp.h"
#include "zbase/EventIngress.h"
#include "zbase/TableDefinition.h"
#include "zbase/JoinedSession.pb.h"
#include "zbase/core/TSDBService.h"
#include "zbase/core/TSDBServlet.h"
#include "zbase/core/ReplicationWorker.h"
#include "csql/defaults.h"
#include "zbase/ConfigDirectory.h"

using namespace stx;
using namespace zbase;

stx::thread::EventLoop ev;

int main(int argc, const char** argv) {
  stx::Application::init();
  stx::Application::logToStderr();

  stx::cli::FlagParser flags;

  flags.defineFlag(
      "http_port",
      stx::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "7004",
      "Start the public http server on this port",
      "<port>");

  flags.defineFlag(
      "frontend",
      cli::FlagParser::T_SWITCH,
      false,
      NULL,
      NULL,
      "frontend only?",
      "<switch>");

  flags.defineFlag(
      "cachedir",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "cachedir path",
      "<path>");

  flags.defineFlag(
      "datadir",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "datadir path",
      "<path>");

  flags.defineFlag(
      "indexbuild_threads",
      cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "2",
      "number of indexbuild threads",
      "<num>");

  flags.defineFlag(
      "master",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "url",
      "<addr>");

  flags.defineFlag(
      "replicate_to",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "url",
      "<addr>");

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

  /* conf */
  //auto conf_data = FileUtil::read(flags.getString("conf"));
  //auto conf = msg::parseText<zbase::TSDBNodeConfig>(conf_data);


  /* thread pools */
  stx::thread::ThreadPool tpool;
  stx::thread::FixedSizeThreadPool wpool(8);
  wpool.start();

  /* http */
  stx::http::HTTPRouter http_router;
  stx::http::HTTPServer http_server(&http_router, &ev);
  http_server.listen(flags.getInt("http_port"));
  http::HTTPConnectionPool http(&ev);

  /* customer directory */
  auto cdb_dir = FileUtil::joinPaths(flags.getString("datadir"), "cdb");
  if (!FileUtil::exists(cdb_dir)) {
    FileUtil::mkdir(cdb_dir);
  }

  ConfigDirectory customer_dir(
      cdb_dir,
      InetAddr::resolve(flags.getString("master")),
      ConfigTopic::CUSTOMERS | ConfigTopic::TABLES | ConfigTopic::USERDB);

  /* dproc */
  auto local_scheduler = mkRef(
      new dproc::LocalScheduler(
          flags.getString("cachedir"),
          12));

  local_scheduler->start();

  /* DocumentDB */
  DocumentDB docdb(flags.getString("datadir"));

  /* tsdb */
  RefPtr<zbase::ReplicationScheme> repl_scheme;

  auto repl_targets = flags.getStrings("replicate_to");
  if (repl_targets.empty()) {
    repl_scheme = RefPtr<zbase::ReplicationScheme>(
        new zbase::StandaloneReplicationScheme());
  } else {
    Vector<zbase::ReplicaRef> replicas;

    for (const auto& r : repl_targets) {
      zbase::ReplicaRef rref(
          SHA1::compute(r),
          InetAddr::resolve(r));

      logInfo(
          "zbase",
          "Adding Remote Replica: $0 ($1)",
          rref.addr.hostAndPort(),
          rref.unique_id.toString());

      replicas.emplace_back(rref);
    }

    if (flags.isSet("frontend")) {
      repl_scheme = RefPtr<zbase::ReplicationScheme>(
          new zbase::FrontendReplicationScheme(replicas));
    } else {
      repl_scheme = RefPtr<zbase::ReplicationScheme>(
          new zbase::FixedReplicationScheme(replicas));
    }
  }

  auto tsdb_dir = FileUtil::joinPaths(flags.getString("datadir"), "tsdb");
  if (!FileUtil::exists(tsdb_dir)) {
    FileUtil::mkdir(tsdb_dir);
  }

  zbase::PartitionMap partition_map(tsdb_dir);
  zbase::TSDBService tsdb_node(&partition_map);
  zbase::ReplicationWorker tsdb_replication(
      repl_scheme.get(),
      &partition_map,
      &http);

  zbase::TSDBServlet tsdb_servlet(&tsdb_node, flags.getString("cachedir"));
  http_router.addRouteByPrefixMatch("/tsdb", &tsdb_servlet, &tpool);

  zbase::CSTableIndex cstable_index(
      &partition_map,
      flags.getInt("indexbuild_threads"));

  /* sql */
  auto sql = csql::Runtime::getDefaultRuntime();
  sql->setCacheDir(flags.getString("cachedir"));

  /* analytics core */
  AnalyticsAuth auth(&customer_dir);

  auto analytics_app = mkRef(
      new AnalyticsApp(
          &tsdb_node,
          &partition_map,
          repl_scheme.get(),
          &cstable_index,
          &customer_dir,
          &auth,
          sql.get(),
          flags.getString("datadir")));

  dproc::DispatchService dproc;
  dproc.registerApp(analytics_app.get(), local_scheduler.get());

  zbase::AnalyticsServlet analytics_servlet(
      analytics_app,
      &dproc,
      nullptr,
      flags.getString("cachedir"),
      &auth,
      sql.get(),
      &tsdb_node,
      &customer_dir,
      &docdb);

  http_router.addRouteByPrefixMatch("/", &analytics_servlet, &tpool);

  {
    FeedConfig fc;
    fc.set_customer("dawanda");
    fc.set_feed("search.ECommerceSearchQueriesFeed");
    fc.set_partition_size(kMicrosPerHour * 4);
    fc.set_first_partition(1430438400000000); // 2015-05-01 00:00:00Z
    fc.set_num_shards(128);
    fc.set_table_name("logjoin.joined_sessions");

    analytics_app->configureFeed(fc);
  }

  {
    FeedConfig fc;
    fc.set_customer("dawanda");
    fc.set_feed("reco_engine.ECommerceRecoQueriesFeed");
    fc.set_partition_size(kMicrosPerHour * 4);
    fc.set_first_partition(1432785600000000);
    fc.set_num_shards(32);
    fc.set_table_name("logjoin.joined_sessions");

    analytics_app->configureFeed(fc);
  }

  {
    FeedConfig fc;
    fc.set_customer("dawanda");
    fc.set_feed("reco_engine.ECommercePreferenceSetsFeed");
    fc.set_partition_size(kMicrosPerHour * 4);
    fc.set_first_partition(1430438400000000); // 2015-05-01 00:00:00Z
    fc.set_num_shards(32);
    fc.set_table_name("logjoin.joined_sessions");

    analytics_app->configureFeed(fc);
  }

  auto rusage_t = std::thread([] () {
    for (;; usleep(1000000)) {
      logDebug(
          "zbase",
          "Using $0MB of memory (peak $1)",
          Application::getCurrentMemoryUsage() / 1000000.0,
          Application::getPeakMemoryUsage() / 1000000.0);
    }
  });

  rusage_t.detach();

  try {
    partition_map.open();
    customer_dir.startWatcher();
    ev.run();
  } catch (const StandardException& e) {
    logAlert("zbase", e, "FATAL ERROR");
  }

  stx::logInfo("zbase", "Exiting...");

  local_scheduler->stop();
  customer_dir.stopWatcher();

  exit(0);
}

