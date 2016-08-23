/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#include "eventql/db/table_config.pb.h"
#include "eventql/db/table_service.h"
#include "eventql/db/metadata_coordinator.h"
#include "eventql/db/metadata_replication.h"
#include "eventql/db/metadata_service.h"
#include "eventql/transport/http/rpc_servlet.h"
#include "eventql/db/replication_worker.h"
#include "eventql/db/tablet_index_cache.h"
#include "eventql/db/compaction_worker.h"
#include "eventql/db/garbage_collector.h"
#include "eventql/db/leader.h"
#include "eventql/db/database.h"
#include "eventql/transport/http/default_servlet.h"
#include "eventql/sql/defaults.h"
#include "eventql/sql/runtime/query_cache.h"
#include "eventql/config/config_directory.h"
#include "eventql/config/config_directory_zookeeper.h"
#include "eventql/config/config_directory_standalone.h"
#include "eventql/transport/http/status_servlet.h"
#include "eventql/server/sql/scheduler.h"
#include "eventql/server/sql/table_provider.h"
#include "eventql/server/listener.h"
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
            "Copyright (c) 2016, DeepCortex GmbH. All rights reserved.\n\n",
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
    config_builder.setProperty("server.client_auth_backend", "trust");
    config_builder.setProperty("server.noleader", "true");
    config_builder.setProperty("cluster.coordinator", "standalone");
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

  /* pidfile */
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

  /* daemonize */
  if (process_config->getBool("server.daemonize")) {
    Application::daemonize();
  }

  /* start database */
  Database db(process_config.get());
  auto rc = db.start();

  /* start listener */
  Listener listener(nullptr);
  if (rc.isSuccess()) {
    rc = listener.bind(listen_port);
  }

  if (rc.isSuccess()) {
    listener.run();
  }

  /* shutdown */
  db.shutdown();

  if (!rc.isSuccess()) {
    logAlert("eventql", "FATAL ERROR: $0", rc.getMessage());
  }

  logInfo("eventql", "Exiting...");

  if (pidfile_lock.get()) {
    pidfile_lock.reset(nullptr);
    FileUtil::rm(process_config->getString("server.pidfile").get());
  }

  exit(rc.isSuccess() ? 0 : 1);
}

