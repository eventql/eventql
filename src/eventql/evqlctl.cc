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
#include <eventql/config/process_config.h>
#include "eventql/master/master_service.h"
#include "eventql/db/server_allocator.h"
#include <eventql/cli/commands/cluster_add_server.h>
#include <eventql/cli/commands/cluster_create.h>
#include <eventql/cli/commands/cluster_status.h>
#include <eventql/cli/commands/cluster_remove_server.h>
#include <eventql/cli/commands/namespace_create.h>
#include <eventql/cli/commands/rebalance.h>

using namespace eventql;

thread::EventLoop ev;

//void cmd_cluster_status(const ::cli::FlagParser& flags) {
  //ConfigDirectoryClient cclient(
  //    InetAddr::resolve(flags.getString("master")));

  //auto cluster = cclient.fetchClusterConfig();
  //iputs("Cluster config:\n$0", cluster.DebugString());
//}

//void cmd_cluster_add_server(const cli::FlagParser& flags) {
//  auto cdir = mkScoped(
//      new ZookeeperConfigDirectory(
//            flags.getString("zookeeper_addr"),
//            None<String>(),
//            ""));
//
//  cdir->startAndJoin(flags.getString("cluster_name"));
//
//  ServerConfig cfg;
//  cfg.set_server_id(flags.getString("server_name"));
//  for (size_t i = 0; i < 128; ++i) {
//    cfg.add_sha1_tokens(Random::singleton()->sha1().toString());
//  }
//
//  cdir->updateServerConfig(cfg);
//
//  cdir->stop();
//}
//
//void cmd_cluster_remove_server(const cli::FlagParser& flags) {
//  bool remove_hard = flags.isSet("hard");
//  bool remove_soft = flags.isSet("soft");
//  if (!(remove_hard ^ remove_soft)) {
//    logFatal("evqlctl", "either --hard or --soft must be set");
//    return;
//  }
//
//  auto cdir = mkScoped(
//      new ZookeeperConfigDirectory(
//            flags.getString("zookeeper_addr"),
//            None<String>(),
//            ""));
//
//  cdir->startAndJoin(flags.getString("cluster_name"));
//
//  auto cfg = cdir->getServerConfig(flags.getString("server_name"));
//  if (remove_soft) {
//    cfg.set_is_leaving(true);
//  }
//  if (remove_hard) {
//    cfg.set_is_dead(true);
//  }
//
//  cdir->updateServerConfig(cfg);
//  cdir->stop();
//}
//
//void cmd_cluster_create(const cli::FlagParser& flags) {
//  auto cdir = mkScoped(
//      new ZookeeperConfigDirectory(
//            flags.getString("zookeeper_addr"),
//            None<String>(),
//            ""));
//
//  cdir->startAndJoin(flags.getString("cluster_name"));
//
//  ClusterConfig cfg;
//  cdir->updateClusterConfig(cfg);
//
//  cdir->stop();
//}
//
//void cmd_namespace_create(const cli::FlagParser& flags) {
//  auto cdir = mkScoped(
//      new ZookeeperConfigDirectory(
//            flags.getString("zookeeper_addr"),
//            None<String>(),
//            ""));
//
//  cdir->startAndJoin(flags.getString("cluster_name"));
//
//  NamespaceConfig cfg;
//  cfg.set_customer(flags.getString("namespace"));
//  cdir->updateNamespaceConfig(cfg);
//
//  cdir->stop();
//}
//
//void cmd_table_split(const cli::FlagParser& flags) {
//  auto cdir = mkScoped(
//      new ZookeeperConfigDirectory(
//            flags.getString("zookeeper_addr"),
//            None<String>(),
//            ""));
//
//  cdir->startAndJoin(flags.getString("cluster_name"));
//
//  auto table_cfg = cdir->getTableConfig(
//      flags.getString("namespace"),
//      flags.getString("table_name"));
//
//  KeyspaceType keyspace;
//  switch (table_cfg.config().partitioner()) {
//    case TBL_PARTITION_FIXED:
//      RAISE(kIllegalArgumentError);
//    case TBL_PARTITION_TIMEWINDOW:
//      keyspace = KEYSPACE_UINT64;
//      break;
//  }
//
//  auto partition_id = SHA1Hash::fromHexString(flags.getString("partition_id"));
//  auto split_partition_id_low = Random::singleton()->sha1();
//  auto split_partition_id_high = Random::singleton()->sha1();
//
//  SplitPartitionOperation op;
//  op.set_partition_id(partition_id.data(), partition_id.size());
//  op.set_split_point(
//      encodePartitionKey(keyspace, flags.getString("split_point")));
//  op.set_split_partition_id_low(
//      split_partition_id_low.data(),
//      split_partition_id_low.size());
//  op.set_split_partition_id_high(
//      split_partition_id_high.data(),
//      split_partition_id_high.size());
//  op.set_placement_id(Random::singleton()->random64());
//
//  ServerAllocator server_alloc(cdir.get());
//
//  Set<String> split_servers_low;
//  {
//    auto rc = server_alloc.allocateServers(3, &split_servers_low);
//    if (!rc.isSuccess()) {
//      logFatal("evqlctl", "ERROR: $0", rc.message());
//      return;
//    }
//  }
//
//  for (const auto& s : split_servers_low) {
//    op.add_split_servers_low(s);
//  }
//
//  Set<String> split_servers_high;
//  {
//    auto rc = server_alloc.allocateServers(3, &split_servers_high);
//    if (!rc.isSuccess()) {
//      logFatal("evqlctl", "ERROR: $0", rc.message());
//      return;
//    }
//  }
//
//  for (const auto& s : split_servers_high) {
//    op.add_split_servers_high(s);
//  }
//
//  MetadataOperation envelope(
//      flags.getString("namespace"),
//      flags.getString("table_name"),
//      METAOP_SPLIT_PARTITION,
//      SHA1Hash(
//          table_cfg.metadata_txnid().data(),
//          table_cfg.metadata_txnid().size()),
//      Random::singleton()->sha1(),
//      *msg::encode(op));
//
//  MetadataCoordinator coordinator(cdir.get());
//  auto rc = coordinator.performAndCommitOperation(
//      flags.getString("namespace"),
//      flags.getString("table_name"),
//      envelope);
//
//  if (!rc.isSuccess()) {
//    logFatal("evqlctl", "ERROR: $0", rc.message());
//  } else {
//    logInfo("evqlctl", "SUCCESS");
//  }
//
//  cdir->stop();
//}
//
//void cmd_rebalance(const cli::FlagParser& flags) {
//  auto cdir = mkScoped(
//      new ZookeeperConfigDirectory(
//            flags.getString("zookeeper_addr"),
//            None<String>(),
//            ""));
//
//  cdir->startAndJoin(flags.getString("cluster_name"));
//  MasterService master(cdir.get());
//  auto rc = master.runOnce();
//  cdir->stop();
//
//  if (!rc.isSuccess()) {
//    logFatal("evqlctl", "ERROR: $0", rc.message());
//  } else {
//    logInfo("evqlctl", "SUCCESS");
//  }
//}

String getCommandName(Vector<String> cmd_argv) {
  String cmd_name;
  if (cmd_argv.size() == 0) {
    return cmd_name;
  }

  cmd_name = cmd_argv[0];
  if (StringUtil::beginsWith(cmd_name, "--")) {
    cmd_name.erase(0, 2);
  } if (StringUtil::beginsWith(cmd_name, "-")) {
    cmd_name.erase(cmd_name.begin());
  }

  return cmd_name;
}

int main(int argc, const char** argv) {
  ::cli::FlagParser flags;
  flags.defineFlag(
      "loglevel",
      ::cli::FlagParser::T_STRING,
      false,
      NULL,
      "INFO",
      "loglevel",
      "<level>");

  flags.defineFlag(
      "c",
      ::cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "path to config file",
      "<config_file>");

  flags.parseArgv(argc, argv);
  auto stdin_is = FileInputStream::getStdin();
  auto stdout_os = OutputStream::getStdout();
  auto stderr_os = OutputStream::getStderr();
  ProcessConfigBuilder config_builder;

  if (flags.isSet("c")) {
    auto rc = config_builder.loadFile(flags.getString("c"));
    if (!rc.isSuccess()) {
      stderr_os->write(rc.message());
      return 1;
    }
  } else {
    auto rc = config_builder.loadDefaultConfigFile();
    if (!rc.isSuccess()) {
      stderr_os->write("Warning: could not load config file");
    }
  }

  auto process_config = config_builder.getConfig();

  List<eventql::cli::CLICommand*> commands;
  commands.emplace_back(new eventql::cli::ClusterAddServer(process_config));
  commands.emplace_back(new eventql::cli::ClusterCreate(process_config));
  commands.emplace_back(new eventql::cli::ClusterRemoveServer(process_config));
  commands.emplace_back(new eventql::cli::ClusterStatus(process_config));
  commands.emplace_back(new eventql::cli::NamespaceCreate(process_config));
  commands.emplace_back(new eventql::cli::Rebalance(process_config));

  Vector<String> cmd_argv = flags.getArgv();
  String cmd_name = getCommandName(cmd_argv);
  if (cmd_name.empty()) {
    //if (default_cmd_.empty()) {
    //  RAISE(kUsageError, "no command provided");
    //}

    //cmd_name = default_cmd_;
    stderr_os->write("UsageError: no command provided\n");
    return 1;

  }

  cmd_argv.erase(cmd_argv.begin());

  if (cmd_name == "help") {
    cmd_name = getCommandName(cmd_argv);
    if (cmd_name.empty()) {
      stdout_os->write(
        "Usage: $ evqlctl [--help] [-C <path>] [-c name=value] \n"
        "<command> [<args>]"
      );
      return 0;
    }

    for (auto c : commands) {
      if (c->getName() == cmd_name) {
        c->printHelp(stdout_os.get());
        return 0;
      }
    }

    stderr_os->write(
        StringUtil::format("No manual entry for evqlctl $0\n", cmd_name));
    return 1;
  }

  for (auto c : commands) {
    if (c->getName() == cmd_name) {
      auto rc = c->execute(
          cmd_argv,
          stdin_is.get(),
          stdout_os.get(),
          stderr_os.get());
      return rc.isSuccess() ? 0 : 1;
    }
  }

  stderr_os->write(StringUtil::format(
      "UsageError: unknown command '$0'",
      cmd_name));

  return 1;
  //erster argv ist commando

  //Application::init();
  //Application::logToStderr("evqlctl");



  //flags.parseArgv(argc, argv);

  //Logger::get()->setMinimumLogLevel(
  //    strToLogLevel(flags.getString("loglevel")));

  //cli::CLI cli;

  ///* command: cluster_status */
  //auto cluster_status_cmd = cli.defineCommand("cluster-status");
  //cluster_status_cmd->onCall(
  //    std::bind(&cmd_cluster_status, std::placeholders::_1));

  //cluster_status_cmd->flags().defineFlag(
  //    "master",
  //    cli::FlagParser::T_STRING,
  //    true,
  //    NULL,
  //    NULL,
  //    "url",
  //    "<addr>");

  ///* command: cluster_add_server */
  //auto cluster_add_server_cmd = cli.defineCommand("cluster-add-server");
  //cluster_add_server_cmd->onCall(
  //    std::bind(&cmd_cluster_add_server, std::placeholders::_1));

  //cluster_add_server_cmd->flags().defineFlag(
  //    "zookeeper_addr",
  //    cli::FlagParser::T_STRING,
  //    false,
  //    NULL,
  //    NULL,
  //    "url",
  //    "<addr>");

  //cluster_add_server_cmd->flags().defineFlag(
  //    "cluster_name",
  //    cli::FlagParser::T_STRING,
  //    true,
  //    NULL,
  //    NULL,
  //    "node name",
  //    "<string>");

  //cluster_add_server_cmd->flags().defineFlag(
  //    "server_name",
  //    cli::FlagParser::T_STRING,
  //    true,
  //    NULL,
  //    NULL,
  //    "node name",
  //    "<string>");

  ///* command: cluster_remove_server */
  //auto cluster_remove_server_cmd = cli.defineCommand("cluster-remove-server");
  //cluster_remove_server_cmd->onCall(
  //    std::bind(&cmd_cluster_remove_server, std::placeholders::_1));

  //cluster_remove_server_cmd->flags().defineFlag(
  //    "zookeeper_addr",
  //    cli::FlagParser::T_STRING,
  //    false,
  //    NULL,
  //    NULL,
  //    "url",
  //    "<remover>");

  //cluster_remove_server_cmd->flags().defineFlag(
  //    "cluster_name",
  //    cli::FlagParser::T_STRING,
  //    true,
  //    NULL,
  //    NULL,
  //    "node name",
  //    "<string>");

  //cluster_remove_server_cmd->flags().defineFlag(
  //    "server_name",
  //    cli::FlagParser::T_STRING,
  //    true,
  //    NULL,
  //    NULL,
  //    "node name",
  //    "<string>");

  //cluster_remove_server_cmd->flags().defineFlag(
  //    "soft",
  //    cli::FlagParser::T_SWITCH,
  //    false,
  //    NULL,
  //    NULL,
  //    "switch",
  //    "<switch>");

  //cluster_remove_server_cmd->flags().defineFlag(
  //    "hard",
  //    cli::FlagParser::T_SWITCH,
  //    false,
  //    NULL,
  //    NULL,
  //    "switch",
  //    "<switch>");

  ///* command: cluster-create */
  //auto cluster_create_node_cmd = cli.defineCommand("cluster-create");
  //cluster_create_node_cmd->onCall(
  //    std::bind(&cmd_cluster_create, std::placeholders::_1));

  //cluster_create_node_cmd->flags().defineFlag(
  //    "zookeeper_addr",
  //    cli::FlagParser::T_STRING,
  //    false,
  //    NULL,
  //    NULL,
  //    "url",
  //    "<addr>");

  //cluster_create_node_cmd->flags().defineFlag(
  //    "cluster_name",
  //    cli::FlagParser::T_STRING,
  //    true,
  //    NULL,
  //    NULL,
  //    "node name",
  //    "<string>");

  ///* command: namespace-create */
  //auto namespace_create_node_cmd = cli.defineCommand("namespace-create");
  //namespace_create_node_cmd->onCall(
  //    std::bind(&cmd_namespace_create, std::placeholders::_1));

  //namespace_create_node_cmd->flags().defineFlag(
  //    "zookeeper_addr",
  //    cli::FlagParser::T_STRING,
  //    false,
  //    NULL,
  //    NULL,
  //    "url",
  //    "<addr>");

  //namespace_create_node_cmd->flags().defineFlag(
  //    "cluster_name",
  //    cli::FlagParser::T_STRING,
  //    true,
  //    NULL,
  //    NULL,
  //    "node name",
  //    "<string>");

  //namespace_create_node_cmd->flags().defineFlag(
  //    "namespace",
  //    cli::FlagParser::T_STRING,
  //    true,
  //    NULL,
  //    NULL,
  //    "node name",
  //    "<string>");

  ///* command: table-split */
  //auto table_split_node_cmd = cli.defineCommand("table-split");
  //table_split_node_cmd->onCall(
  //    std::bind(&cmd_table_split, std::placeholders::_1));

  //table_split_node_cmd->flags().defineFlag(
  //    "zookeeper_addr",
  //    cli::FlagParser::T_STRING,
  //    false,
  //    NULL,
  //    NULL,
  //    "url",
  //    "<addr>");

  //table_split_node_cmd->flags().defineFlag(
  //    "cluster_name",
  //    cli::FlagParser::T_STRING,
  //    true,
  //    NULL,
  //    NULL,
  //    "node name",
  //    "<string>");

  //table_split_node_cmd->flags().defineFlag(
  //    "namespace",
  //    cli::FlagParser::T_STRING,
  //    true,
  //    NULL,
  //    NULL,
  //    "namespace",
  //    "<string>");

  //table_split_node_cmd->flags().defineFlag(
  //    "table_name",
  //    cli::FlagParser::T_STRING,
  //    true,
  //    NULL,
  //    NULL,
  //    "table name",
  //    "<string>");

  //table_split_node_cmd->flags().defineFlag(
  //    "partition_id",
  //    cli::FlagParser::T_STRING,
  //    true,
  //    NULL,
  //    NULL,
  //    "table name",
  //    "<string>");

  //table_split_node_cmd->flags().defineFlag(
  //    "split_point",
  //    cli::FlagParser::T_STRING,
  //    true,
  //    NULL,
  //    NULL,
  //    "table name",
  //    "<string>");

  ///* command: rebalance */
  //auto rebalance_node_cmd = cli.defineCommand("rebalance");
  //rebalance_node_cmd->onCall(
  //    std::bind(&cmd_rebalance, std::placeholders::_1));

  //rebalance_node_cmd->flags().defineFlag(
  //    "zookeeper_addr",
  //    cli::FlagParser::T_STRING,
  //    false,
  //    NULL,
  //    NULL,
  //    "url",
  //    "<addr>");

  //rebalance_node_cmd->flags().defineFlag(
  //    "cluster_name",
  //    cli::FlagParser::T_STRING,
  //    true,
  //    NULL,
  //    NULL,
  //    "node name",
  //    "<string>");

  //cli.call(flags.getArgv());
  return 0;
}

