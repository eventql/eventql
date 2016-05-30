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
#include "eventql/eventql.h"
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "eventql/util/io/filerepository.h"
#include "eventql/util/io/fileutil.h"
#include "eventql/util/application.h"
#include "eventql/util/logging.h"
#include "eventql/util/random.h"
#include "eventql/util/thread/eventloop.h"
#include "eventql/util/thread/threadpool.h"
#include "eventql/util/thread/FixedSizeThreadPool.h"
#include "eventql/util/wallclock.h"
#include "eventql/util/VFS.h"
#include "eventql/util/cli/flagparser.h"
#include "eventql/util/json/json.h"
#include "eventql/util/json/jsonrpc.h"
#include "eventql/util/http/httprouter.h"
#include "eventql/util/http/httpserver.h"
#include "eventql/util/http/httpconnectionpool.h"
#include "eventql/util/http/VFSFileServlet.h"
#include "eventql/util/cli/CLI.h"
#include "eventql/util/cli/flagparser.h"
#include "eventql/config/config_directory.h"
#include "eventql/config/config_directory_zookeeper.h"

using namespace eventql;

thread::EventLoop ev;

void cmd_cluster_status(const cli::FlagParser& flags) {
  //ConfigDirectoryClient cclient(
  //    InetAddr::resolve(flags.getString("master")));

  //auto cluster = cclient.fetchClusterConfig();
  //iputs("Cluster config:\n$0", cluster.DebugString());
}

void cmd_cluster_add_server(const cli::FlagParser& flags) {
  auto cdir = mkScoped(
      new ZookeeperConfigDirectory(
            flags.getString("zookeeper_addr"),
            None<String>(),
            ""));

  cdir->startAndJoin(flags.getString("cluster_name"));

  ServerConfig cfg;
  cfg.set_server_id(flags.getString("server_name"));
  cfg.add_sha1_tokens(Random::singleton()->sha1().toString());

  cdir->updateServerConfig(cfg);

  cdir->stop();
}

void cmd_cluster_create(const cli::FlagParser& flags) {
  auto cdir = mkScoped(
      new ZookeeperConfigDirectory(
            flags.getString("zookeeper_addr"),
            None<String>(),
            ""));

  cdir->startAndJoin(flags.getString("cluster_name"));

  ClusterConfig cfg;
  cdir->updateClusterConfig(cfg);

  cdir->stop();
}

void cmd_namespace_create(const cli::FlagParser& flags) {
  auto cdir = mkScoped(
      new ZookeeperConfigDirectory(
            flags.getString("zookeeper_addr"),
            None<String>(),
            ""));

  cdir->startAndJoin(flags.getString("cluster_name"));

  NamespaceConfig cfg;
  cfg.set_customer(flags.getString("namespace"));
  cdir->updateNamespaceConfig(cfg);

  cdir->stop();
}

int main(int argc, const char** argv) {
  Application::init();
  Application::logToStderr("evqlctl");

  cli::FlagParser flags;

  flags.defineFlag(
      "loglevel",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      "INFO",
      "loglevel",
      "<level>");

  flags.parseArgv(argc, argv);

  Logger::get()->setMinimumLogLevel(
      strToLogLevel(flags.getString("loglevel")));

  cli::CLI cli;

  /* command: cluster_status */
  auto cluster_status_cmd = cli.defineCommand("cluster-status");
  cluster_status_cmd->onCall(
      std::bind(&cmd_cluster_status, std::placeholders::_1));

  cluster_status_cmd->flags().defineFlag(
      "master",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "url",
      "<addr>");

  /* command: cluster_add_server */
  auto cluster_add_server_cmd = cli.defineCommand("cluster-add-server");
  cluster_add_server_cmd->onCall(
      std::bind(&cmd_cluster_add_server, std::placeholders::_1));

  cluster_add_server_cmd->flags().defineFlag(
      "zookeeper_addr",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "url",
      "<addr>");

  cluster_add_server_cmd->flags().defineFlag(
      "cluster_name",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "node name",
      "<string>");

  cluster_add_server_cmd->flags().defineFlag(
      "server_name",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "node name",
      "<string>");

  /* command: cluster-create */
  auto cluster_create_node_cmd = cli.defineCommand("cluster-create");
  cluster_create_node_cmd->onCall(
      std::bind(&cmd_cluster_create, std::placeholders::_1));

  cluster_create_node_cmd->flags().defineFlag(
      "zookeeper_addr",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "url",
      "<addr>");

  cluster_create_node_cmd->flags().defineFlag(
      "cluster_name",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "node name",
      "<string>");

  /* command: namespace-create */
  auto namespace_create_node_cmd = cli.defineCommand("namespace-create");
  namespace_create_node_cmd->onCall(
      std::bind(&cmd_namespace_create, std::placeholders::_1));

  namespace_create_node_cmd->flags().defineFlag(
      "zookeeper_addr",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "url",
      "<addr>");

  namespace_create_node_cmd->flags().defineFlag(
      "cluster_name",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "node name",
      "<string>");

  namespace_create_node_cmd->flags().defineFlag(
      "namespace",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "node name",
      "<string>");

  cli.call(flags.getArgv());
  return 0;
}

