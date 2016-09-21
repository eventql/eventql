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
#include "eventql/eventql.h"
#include "eventql/util/application.h"
#include "eventql/util/logging.h"
#include "eventql/util/cli/flagparser.h"
#include "eventql/util/io/FileLock.h"
#include "eventql/util/io/fileutil.h"

int shutdown_pipe[2];
void shutdown(int);

int main(int argc, const char** argv) {
  Application::init();
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
#ifdef EVQL_VERSION
    static const std::string version = EVQL_VERSION;
#else
    static const std::string version = "unknown";
#endif

#ifdef EVQL_BUILDID
    static const std::string build_id = EVQL_BUILDID;
#else
    static const std::string build_id = "unknown";
#endif

    auto stdout_os = OutputStream::getStdout();
    stdout_os->write(
        StringUtil::format(
            "EventQL $0 ($1)\n"
            "Copyright (c) 2016, DeepCortex GmbH. All rights reserved.\n\n",
            version,
            build_id));
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
  auto conf = evql_conf_init();
  if (!conf) {
    logCritical("evqld", "error while initializing EventQL server");
    return 1;
  }

  evql_conf_set(conf, "cluster.rebalance_interval", "60000000");
  evql_conf_set(conf, "cluster.allow_anonymous", "true");
  evql_conf_set(conf, "cluster.allow_drop_table", "false");
  evql_conf_set(conf, "server.listen", "localhost:9175");
  evql_conf_set(conf, "server.indexbuild_threads", "2");
  evql_conf_set(conf, "server.gc_mode", "MANUAL");
  evql_conf_set(conf, "server.gc_interval", "30000000");
  evql_conf_set(conf, "server.cachedir_maxsize", "68719476736");
  evql_conf_set(conf, "server.noleader", "false");
  evql_conf_set(conf, "server.c2s_io_timeout", "1000000");
  evql_conf_set(conf, "server.c2s_idle_timeout", "1800000000");
  evql_conf_set(conf, "server.s2s_io_timeout", "1000000");
  evql_conf_set(conf, "server.s2s_idle_timeout", "5000000");
  evql_conf_set(conf, "server.http_io_timeout", "1000000");
  evql_conf_set(conf, "server.heartbeat_interval", "1000000");
  evql_conf_set(conf, "server.query_progress_rate_limit", "250000");
  evql_conf_set(conf, "server.query_max_concurrent_shards", "256");
  evql_conf_set(conf, "server.query_max_concurrent_shards_per_host", "4");
  evql_conf_set(conf, "server.query_failed_shard_policy", "tolerate");
  evql_conf_set(conf, "server.loadinfo_publish_interval", "900000000");

  if (flags.isSet("standalone")) {
    evql_conf_set(conf, "cluster.coordinator", "standalone");
    evql_conf_set(conf, "cluster.allowed_hosts", "0.0.0.0/0");
    evql_conf_set(conf, "server.name", "standalone");
    evql_conf_set(conf, "server.client_auth_backend", "trust");
    evql_conf_set(conf, "server.noleader", "true");
  }

  if (flags.isSet("config")) {
    auto config_file_path = flags.getString("config");
    int rc = evql_conf_load(
        conf,
        config_file_path.empty() ? nullptr : config_file_path.c_str());

    if (rc) {
      logCritical(
          "evqld",
          "error while loading config file");

      evql_conf_free(conf);
      return 1;
    }
  }

  for (const auto& opt : flags.getStrings("config_set")) {
    auto opt_key_end = opt.find("=");
    if (opt_key_end == String::npos) {
      logCritical("invalid config option: $0", opt);
      evql_conf_free(conf);
      return 1;
    }

    auto opt_key = opt.substr(0, opt_key_end);
    auto opt_value = opt.substr(opt_key_end + 1);
    evql_conf_set(conf, opt_key.c_str(), opt_value.c_str());
  }

  if (flags.isSet("listen")) {
    auto listen = flags.getString("listen");
    evql_conf_set(conf, "server.listen", listen.c_str());
  }

  if (flags.isSet("datadir")) {
    auto datadir = flags.getString("datadir");
    evql_conf_set(conf, "server.datadir", datadir.c_str());
  }

  if (flags.isSet("daemonize")) {
    evql_conf_set(conf, "server.daemonize", "true");
  }

  if (flags.isSet("pidfile")) {
    auto pidfile = flags.getString("pidfile");
    evql_conf_set(conf, "server.pidfile", pidfile.c_str());
  }

  /* init shutdown handler */
  if (pipe(shutdown_pipe) != 0) {
    logCritical("evqld", "error while initializing evqld: pipe failed()");
    return 1;
  }

  signal(SIGTERM, shutdown);
  signal(SIGINT, shutdown);
  signal(SIGHUP, shutdown);
  signal(SIGPIPE, SIG_IGN);

  /* init server */
  auto server = evql_server_init(conf);
  if (!server) {
    logCritical("evqld", "error while initializing EventQL server");
    evql_conf_free(conf);
    return 1;
  }

  if (!evql_server_getconf(server, "server.datadir")) {
    logCritical("evqld", "missing 'server.datadir' option or --datadir flag");
    evql_server_free(server);
    evql_conf_free(conf);
    return 1;
  }

  /* daemonize */
  if (evql_server_getconfbool(server, "server.daemonize")) {
    Application::daemonize();
  }

  /* pidfile */
  ScopedPtr<FileLock> pidfile_lock;
  String pidfile_path;
  {
    auto pidfile_path_cstr = evql_server_getconf(server, "server.pidfile");
    if (pidfile_path_cstr) {
      pidfile_path = std::string(pidfile_path_cstr);
    }
  }

  if (!pidfile_path.empty()) {
    pidfile_lock = mkScoped(new FileLock(pidfile_path));
    pidfile_lock->lock(false);

    auto pidfile = File::openFile(
        pidfile_path,
        File::O_WRITE | File::O_CREATEOROPEN | File::O_TRUNCATE);

    pidfile.write(StringUtil::toString(getpid()));
  }

  /* start database */
  int rc = evql_server_start(server);
  if (!rc) {
    rc = evql_server_listen(server, shutdown_pipe[0]);
  }

  if (rc) {
    logCritical("eventql", evql_server_geterror(server));
  }

  /* shutdown */
  logInfo("eventql", "Exiting...");
  evql_server_shutdown(server);

  signal(SIGTERM, SIG_IGN);
  signal(SIGINT, SIG_IGN);
  signal(SIGHUP, SIG_IGN);
  close(shutdown_pipe[0]);
  close(shutdown_pipe[1]);

  if (pidfile_lock.get()) {
    pidfile_lock.reset(nullptr);
    FileUtil::rm(pidfile_path);
  }

  evql_server_free(server);
  evql_conf_free(conf);

  return rc;
}

void shutdown(int) {
  unsigned char one = 1;
  write(shutdown_pipe[1], &one, 1);
}

