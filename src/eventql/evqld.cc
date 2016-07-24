/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <regex>
#include <sys/time.h>
#include <sys/resource.h>
#include "eventql/util/io/filerepository.h"
#include "eventql/util/io/fileutil.h"
#include "eventql/util/application.h"
#include "eventql/util/logging.h"
#include "eventql/util/random.h"
#include "eventql/util/assets.h"
#include "eventql/util/thread/eventloop.h"
#include "eventql/util/thread/threadpool.h"
#include "eventql/util/thread/FixedSizeThreadPool.h"
#include "eventql/util/wallclock.h"
#include "eventql/util/VFS.h"
#include "eventql/util/rpc/ServerGroup.h"
#include "eventql/util/rpc/RPC.h"
#include "eventql/util/rpc/RPCClient.h"
#include "eventql/util/cli/flagparser.h"
#include "eventql/util/json/json.h"
#include "eventql/util/json/jsonrpc.h"
#include "eventql/util/http/httprouter.h"
#include "eventql/util/http/httpserver.h"
#include "eventql/util/http/VFSFileServlet.h"
#include "eventql/util/io/FileLock.h"
#include "eventql/util/stats/statsdagent.h"
#include "eventql/io/sstable/SSTableServlet.h"
#include "eventql/util/mdb/MDB.h"
#include "eventql/util/mdb/MDBUtil.h"
#include "eventql/transport/http/api_servlet.h"
#include "eventql/db/TableConfig.pb.h"
#include "eventql/db/table_service.h"
#include "eventql/db/metadata_coordinator.h"
#include "eventql/db/metadata_replication.h"
#include "eventql/db/metadata_service.h"
#include "eventql/transport/http/rpc_servlet.h"
#include "eventql/db/ReplicationWorker.h"
#include "eventql/db/LSMTableIndexCache.h"
#include "eventql/db/CompactionWorker.h"
#include "eventql/db/garbage_collector.h"
#include "eventql/db/leader.h"
#include "eventql/server/sql/sql_engine.h"
#include "eventql/transport/http/default_servlet.h"
#include "eventql/sql/defaults.h"
#include "eventql/sql/runtime/query_cache.h"
#include "eventql/config/config_directory.h"
#include "eventql/config/config_directory_legacy.h"
#include "eventql/config/config_directory_zookeeper.h"
#include "eventql/config/config_directory_standalone.h"
#include "eventql/transport/http/status_servlet.h"
#include "eventql/server/sql/scheduler.h"
#include "eventql/auth/client_auth.h"
#include "eventql/auth/client_auth_trust.h"
#include "eventql/auth/client_auth_legacy.h"
#include "eventql/auth/internal_auth.h"
#include "eventql/auth/internal_auth_trust.h"
#include <jsapi.h>
#include "eventql/mapreduce/mapreduce_preludejs.cc"
#include "eventql/eventql.h"
#include "eventql/db/file_tracker.h"

using namespace eventql;

thread::EventLoop ev;

namespace js {
void DisableExtraThreads();
}

int main(int argc, const char** argv) {
  Application::init();
  __eventql_mapreduce_prelude_js.registerAsset();

  cli::FlagParser flags;

  flags.defineFlag(
      "help",
      cli::FlagParser::T_SWITCH,
      false,
      "?",
      NULL,
      "help",
      "<help>");

  flags.defineFlag(
      "version",
      cli::FlagParser::T_SWITCH,
      false,
      "V",
      NULL,
      "print version",
      "<switch>");

  flags.defineFlag(
      "config",
      ::cli::FlagParser::T_STRING,
      false,
      "c",
      NULL,
      "path to config file",
      "<config_file>");

  flags.defineFlag(
      "config_set",
      ::cli::FlagParser::T_STRING,
      false,
      "C",
      NULL,
      "set config option",
      "<key>=<val>");

  flags.defineFlag(
      "standalone",
      cli::FlagParser::T_SWITCH,
      false,
      NULL,
      NULL,
      "standalone mode",
      "<switch>");

  flags.defineFlag(
      "listen",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "listen",
      "<host:port>");

  flags.defineFlag(
      "datadir",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "datadir path",
      "<path>");

  flags.defineFlag(
      "loglevel",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      "INFO",
      "loglevel",
      "<level>");

  flags.defineFlag(
      "daemonize",
      cli::FlagParser::T_SWITCH,
      false,
      NULL,
      NULL,
      "daemonize",
      "<switch>");

  flags.defineFlag(
      "pidfile",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "pidfile",
      "<path>");

  flags.defineFlag(
      "log_to_syslog",
      cli::FlagParser::T_SWITCH,
      false,
      NULL,
      NULL,
      "log to syslog",
      "<switch>");

  flags.defineFlag(
      "nolog_to_stderr",
      cli::FlagParser::T_SWITCH,
      false,
      NULL,
      NULL,
      "don't log to stderr",
      "<switch>");

  flags.parseArgv(argc, argv);

  if (!flags.isSet("nolog_to_stderr") && !flags.isSet("daemonize")) {
    Application::logToStderr("evqld");
  }

  if (flags.isSet("log_to_syslog")) {
    Application::logToSyslog("evqld");
  }

  Logger::get()->setMinimumLogLevel(
      strToLogLevel(flags.getString("loglevel")));

  /* print help */
  if (flags.isSet("help") || flags.isSet("version")) {
    auto stdout_os = OutputStream::getStdout();
    stdout_os->write(
        StringUtil::format(
            "EventQL $0 ($1)\n"
            "Copyright (c) 2016, zScale Techology GmbH. All rights reserved.\n\n",
            kVersionString,
            kBuildID));
  }

  if (flags.isSet("version")) {
    return 0;
  }

  if (flags.isSet("help")) {
    auto stdout_os = OutputStream::getStdout();
    stdout_os->write(
        "Usage: $ evqld [OPTIONS]\n\n"
        "   -c, --config <file>       Load config from file\n"
        "   -C name=value             Define a config value on the command line\n"
        "   --standalone              Run in standalone mode\n"
        "   --datadir <path>          Path to data directory\n"
        "   --listen <host:port>      Listen on this address (default: localhost:9175)\n"
        "   --daemonize               Daemonize the server\n"
        "   --pidfile <file>          Write a PID file\n"
        "   --loglevel <level>        Minimum log level (default: INFO)\n"
        "   --[no]log_to_syslog       Do[n't] log to syslog\n"
        "   --[no]log_to_stderr       Do[n't] log to stderr\n"
        "   -?, --help                Display this help text and exit\n"
        "   -v, --version             Display the version of this binary and exit\n"
        "                                                       \n"
        "Examples:                                              \n"
        "   $ evqld --standalone --datadir /var/evql\n"
        "   $ evqld -c /etc/evqld.conf --daemonize --pidfile /run/evql.pid\n"
    );
    return 0;
  }

  /* conf */
  ProcessConfigBuilder config_builder;
  config_builder.setProperty("server.listen", "localhost:9175");
  config_builder.setProperty("server.indexbuild_threads", "2");
  config_builder.setProperty("server.gc_mode", "AUTOMATIC");
  config_builder.setProperty("server.gc_interval", "30000000");
  config_builder.setProperty("server.cachedir_maxsize", "68719476736");
  config_builder.setProperty("server.noleader", "false");
  config_builder.setProperty("cluster.rebalance_interval", "60000000");

  if (flags.isSet("standalone")) {
    config_builder.setProperty("server.name", "standalone");
    config_builder.setProperty("cluster.coordinator", "standalone");
    config_builder.setProperty("server.client_auth_backend", "trust");
  }

  if (flags.isSet("config")) {
    auto rc = config_builder.loadFile(flags.getString("config"));
    if (!rc.isSuccess()) {
      logFatal("error while loading config file: $0", rc.message());
      return 1;
    }
  } else {
    config_builder.loadDefaultConfigFile("evqld");
  }

  for (const auto& opt : flags.getStrings("config_set")) {
    auto opt_key_end = opt.find("=");
    if (opt_key_end == String::npos) {
      logFatal("invalid config option: $0", opt);
      return 1;
    }

    config_builder.setProperty(
        opt.substr(0, opt_key_end),
        opt.substr(opt_key_end + 1));
  }

  if (flags.isSet("listen")) {
    config_builder.setProperty("server.listen", flags.getString("listen"));
  }

  if (flags.isSet("datadir")) {
    config_builder.setProperty("server.datadir", flags.getString("datadir"));
  }

  if (flags.isSet("daemonize")) {
    config_builder.setProperty("server.daemonize", "true");
  }

  if (flags.isSet("pidfile")) {
    config_builder.setProperty("server.pidfile", flags.getString("pidfile"));
  }

  auto process_config = config_builder.getConfig();

  if (!process_config->hasProperty("server.datadir")) {
    logFatal("evqld", "missing 'server.datadir' option or --datadir flag");
    return 1;
  }

  auto server_datadir = process_config->getString("server.datadir").get();
  if (!FileUtil::exists(server_datadir)) {
    logFatal(
        "evqld",
        "data dir not found: $0",
        server_datadir);
    return 1;
  }

  //if (!process_config->hasProperty("server.name")) {
  //  logFatal("evqld", "missing 'server.name' option");
  //  return 1;
  //}

  /* daemonize */
  if (process_config->getBool("server.daemonize")) {
    Application::daemonize();
  }

  ScopedPtr<FileLock> pidfile_lock;
  if (process_config->hasProperty("server.pidfile")) {
    auto pidfile_path = process_config->getString("server.pidfile").get();
    pidfile_lock = mkScoped(new FileLock(pidfile_path));
    pidfile_lock->lock(false);

    auto pidfile = File::openFile(
        pidfile_path,
        File::O_WRITE | File::O_CREATEOROPEN | File::O_TRUNCATE);

    pidfile.write(StringUtil::toString(getpid()));
  }

  /* thread pools */
  thread::CachedThreadPool tpool(
      thread::ThreadPoolOptions {
        .thread_name = Some(String("evqld-httpserver"))
      },
      mkScoped(new CatchAndLogExceptionHandler("evqld")),
      8);

  /* listen addr */
  String listen_host;
  int listen_port;
  {
    auto listen_str = process_config->getString("server.listen");
    if (listen_str.isEmpty()) {
      logFatal("evqld", "missing 'server.listen' option or --listen flag");
      return 1;
    }

    std::smatch m;
    std::regex listen_regex("([0-9a-zA-Z-_.]+):([0-9]+)");
    if (std::regex_match(listen_str.get(), m, listen_regex)) {
      listen_host = m[1];
      listen_port = std::stoi(m[2]);
    } else {
      logFatal("evqld", "invalid listen address: $0", listen_str.get());
      return 1;
    }
  }

  /* http */
  http::HTTPRouter http_router;
  http::HTTPServer http_server(&http_router, &ev);
  http_server.listen(listen_port);
  http::HTTPConnectionPool http(&ev, &z1stats()->http_client_stats);

  /* data directory */
  auto server_name = process_config->getString("server.name");
  String tsdb_dir;
  String metadata_dir;
  if (server_name.isEmpty()) {
    tsdb_dir = FileUtil::joinPaths(server_datadir, "data/__anonymous");
    metadata_dir = FileUtil::joinPaths(server_datadir, "metadata/__anonymous");
  } else {
    tsdb_dir = FileUtil::joinPaths(
        server_datadir,
        "data/" + server_name.get());
    metadata_dir = FileUtil::joinPaths(
        server_datadir,
        "metadata/" + server_name.get());
  }

  auto trash_dir = FileUtil::joinPaths(server_datadir, "trash");
  auto cache_dir = FileUtil::joinPaths(server_datadir, "cache");

  if (!FileUtil::exists(tsdb_dir)) {
    FileUtil::mkdir_p(tsdb_dir);
  }

  if (!FileUtil::exists(metadata_dir)) {
    FileUtil::mkdir_p(metadata_dir);
  }

  if (!FileUtil::exists(trash_dir)) {
    FileUtil::mkdir(trash_dir);
  }

  if (!FileUtil::exists(cache_dir)) {
    FileUtil::mkdir(cache_dir);
  }

  /* file tracker */
  FileTracker file_tracker(trash_dir);

  /* garbage collector */
  auto gc_mode = garbageCollectorModeFromString(
      process_config->getString("server.gc_mode").get());

  GarbageCollector gc(
      gc_mode,
      &file_tracker,
      tsdb_dir,
      trash_dir,
      cache_dir,
      process_config->getInt("server.cachedir_maxsize").get(),
      process_config->getInt("server.gc_interval").get());

  /* config dir */
  ScopedPtr<ConfigDirectory> config_dir;
  {
    auto rc = ConfigDirectoryFactory::getConfigDirectoryForServer(
        process_config.get(),
        &config_dir);
    if (!rc.isSuccess()) {
      logFatal("evqld", "can't open config backend: $0", rc.message());
      return 1;
    }
  }

  /* client auth */
  if (!process_config->hasProperty("server.client_auth_backend")) {
    logFatal("evqld", "missing 'server.client_auth_backend'");
    return 1;
  }

  ScopedPtr<eventql::ClientAuth> client_auth;
  auto client_auth_opt = process_config->getString("server.client_auth_backend");
  if (client_auth_opt.get() == "trust") {
    client_auth.reset(new TrustClientAuth());
  } else if (client_auth_opt.get() == "legacy") {
    if (!process_config->hasProperty("server.legacy_auth_secret")) {
      logFatal("evqld", "missing 'server.legacy_auth_secret'");
      return 1;
    }

    client_auth.reset(
        new LegacyClientAuth(
            process_config->getString("server.legacy_auth_secret").get()));
  } else {
    logFatal("evqld", "invalid client auth backend: " + client_auth_opt.get());
    return 1;
  }

  /* internal auth */
  ScopedPtr<eventql::InternalAuth> internal_auth;
  internal_auth.reset(new TrustInternalAuth());

  /* metadata service */
  eventql::MetadataStore metadata_store(metadata_dir);
  eventql::MetadataService metadata_service(config_dir.get(), &metadata_store);

  /* leader */
  Leader leader(
      config_dir.get(),
      process_config->getInt("cluster.rebalance_interval").get());

  /* spidermonkey javascript runtime */
  JS_Init();
  js::DisableExtraThreads();

  try {
    {
      auto rc = config_dir->start();
      if (!rc.isSuccess()) {
        logFatal("evqld", "Can't connect to config backend: $0", rc.message());
        return 1;
      }
    }

    /* clusterconfig */
    auto cluster_config = config_dir->getClusterConfig();

    /* tsdb */
    auto repl_scheme = RefPtr<eventql::ReplicationScheme>(
          new eventql::DHTReplicationScheme(cluster_config, server_name));

    FileLock server_lock(FileUtil::joinPaths(tsdb_dir, "__lock"));
    server_lock.lock();

    eventql::ServerCfg cfg;
    cfg.db_path = tsdb_dir;
    cfg.repl_scheme = repl_scheme;
    cfg.config_directory = config_dir.get();
    cfg.idx_cache = mkRef(new LSMTableIndexCache(tsdb_dir));
    cfg.metadata_store = &metadata_store;
    cfg.file_tracker = &file_tracker;

    eventql::PartitionMap partition_map(&cfg);
    eventql::TableService table_service(
        config_dir.get(),
        &partition_map,
        repl_scheme.get(),
        &ev,
        &z1stats()->http_client_stats);

    eventql::ReplicationWorker tsdb_replication(
        repl_scheme.get(),
        &partition_map,
        &http);

    eventql::RPCServlet tsdb_servlet(
        &table_service,
        &metadata_service,
        cache_dir);

    http_router.addRouteByPrefixMatch("/tsdb", &tsdb_servlet, &tpool);
    http_router.addRouteByPrefixMatch("/rpc", &tsdb_servlet, &tpool);

    eventql::CompactionWorker cstable_index(
        &partition_map,
        process_config->getInt("server.indexbuild_threads").get());

    /* metadata replication */
    ScopedPtr<MetadataReplication> metadata_replication;
    if (!server_name.isEmpty()) {
      metadata_replication.reset(
          new MetadataReplication(
              config_dir.get(),
              server_name.get(),
              &metadata_store));
    }

    /* sql */
    csql::QueryCache sql_query_cache(cache_dir);
    RefPtr<csql::Runtime> sql;
    {
      auto symbols = mkRef(new csql::SymbolTable());
      csql::installDefaultSymbols(symbols.get());
      sql = mkRef(new csql::Runtime(
          thread::ThreadPoolOptions {
            .thread_name = Some(String("evqld-sqlruntime"))
          },
          symbols,
          new csql::QueryBuilder(
              new csql::ValueExpressionBuilder(symbols.get())),
          new csql::QueryPlanBuilder(
              csql::QueryPlanBuilderOptions{},
              symbols.get()),
          mkScoped(
              new Scheduler(
                  &partition_map,
                  config_dir.get(),
                  internal_auth.get(),
                  repl_scheme.get()))));

      sql->setCacheDir(cache_dir);
      sql->symbols()->registerFunction("z1_version", &z1VersionExpr);
      sql->setQueryCache(&sql_query_cache);
    }

    /* more services */
    eventql::SQLService sql_service(
        sql.get(),
        &partition_map,
        config_dir.get(),
        repl_scheme.get(),
        internal_auth.get(),
        &table_service,
        cache_dir);

    eventql::LogfileService logfile_service(
        config_dir.get(),
        internal_auth.get(),
        &table_service,
        &partition_map,
        repl_scheme.get(),
        sql.get());

    eventql::MapReduceService mapreduce_service(
        config_dir.get(),
        internal_auth.get(),
        &table_service,
        &partition_map,
        repl_scheme.get(),
        cache_dir);

    /* open tables */
    config_dir->setTableConfigChangeCallback(
        [&partition_map, &tsdb_replication] (const TableDefinition& tbl) {
      Set<SHA1Hash> affected_partitions;
      try {
        partition_map.configureTable(tbl, &affected_partitions);
      } catch (const std::exception& e) {
        logError(
            "evqld",
            "error while applying table config change to $0/$1",
            tbl.customer(),
            tbl.table_name());
      }

      for (const auto& partition_id : affected_partitions) {
        try {
          auto partition = partition_map.findPartition(
              tbl.customer(),
              tbl.table_name(),
              partition_id);

          tsdb_replication.enqueuePartition(partition.get(), 0);
        } catch (const std::exception& e) {
          logError(
              "evqld",
              "error while applying table config change to $0/$1/$2",
              tbl.customer(),
              tbl.table_name(),
              partition_id.toString());
        }
      }
    });

    config_dir->listTables([&partition_map] (const TableDefinition& tbl) {
      partition_map.configureTable(tbl);
    });

    eventql::AnalyticsServlet analytics_servlet(
        cache_dir,
        internal_auth.get(),
        client_auth.get(),
        internal_auth.get(),
        sql.get(),
        &table_service,
        config_dir.get(),
        &partition_map,
        &sql_service,
        &logfile_service,
        &mapreduce_service,
        &table_service);

    eventql::StatusServlet status_servlet(
        &cfg,
        &partition_map,
        config_dir.get(),
        http_server.stats(),
        &z1stats()->http_client_stats,
        &tsdb_replication);

    eventql::DefaultServlet default_servlet;

    http_router.addRouteByPrefixMatch("/api/", &analytics_servlet, &tpool);
    http_router.addRouteByPrefixMatch("/zstatus", &status_servlet, &tpool);
    http_router.addRouteByPrefixMatch("/", &default_servlet);

    auto rusage_t = std::thread([] () {
      for (;; usleep(1000000)) {
        logDebug(
            "eventql",
            "Using $0MB of memory (peak $1)",
            Application::getCurrentMemoryUsage() / 1000000.0,
            Application::getPeakMemoryUsage() / 1000000.0);
      }
    });

    rusage_t.detach();

    Application::setCurrentThreadName("evqld");

    partition_map.open();
    gc.startGCThread();

    if (!process_config->getBool("server.noleader")) {
      leader.startLeaderThread();
    }

    if (metadata_replication.get()) {
      metadata_replication->start();
    }

    ev.run();

    if (metadata_replication.get()) {
      metadata_replication->stop();
    }

    leader.stopLeaderThread();
    gc.stopGCThread();
  } catch (const StandardException& e) {
    logAlert("eventql", e, "FATAL ERROR");
  }

  logInfo("eventql", "Exiting...");
  config_dir->stop();
  JS_ShutDown();

  if (pidfile_lock.get()) {
    pidfile_lock.reset(nullptr);
    FileUtil::rm(process_config->getString("server.pidfile").get());
  }

  exit(0);
}

