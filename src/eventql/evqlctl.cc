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
#include "eventql/dproc/LocalScheduler.h"
#include "eventql/dproc/DispatchService.h"
#include "eventql/ConfigDirectory.h"
#include "eventql/infra/sstable/sstablereader.h"
#include "eventql/core/TimeWindowPartitioner.h"
#include "eventql/core/TSDBClient.h"
#include "csql/qtree/SequentialScanNode.h"
#include "csql/qtree/ColumnReferenceNode.h"
#include "csql/qtree/CallExpressionNode.h"
#include "csql/qtree/GroupByNode.h"
#include "csql/qtree/UnionNode.h"
#include "csql/runtime/queryplanbuilder.h"
#include "csql/CSTableScan.h"
#include "csql/CSTableScanProvider.h"
#include "csql/runtime/defaultruntime.h"
#include "csql/runtime/tablerepository.h"

using namespace stx;
using namespace zbase;

stx::thread::EventLoop ev;

void cmd_cluster_status(const cli::FlagParser& flags) {
  ConfigDirectoryClient cclient(
      InetAddr::resolve(flags.getString("master")));

  auto cluster = cclient.fetchClusterConfig();
  iputs("Cluster config:\n$0", cluster.DebugString());
}

void cmd_cluster_add_node(const cli::FlagParser& flags) {
  ConfigDirectoryClient cclient(
      InetAddr::resolve(flags.getString("master")));

  auto cluster = cclient.fetchClusterConfig();
  auto node = cluster.add_dht_nodes();
  node->set_name(flags.getString("name"));
  node->set_addr(flags.getString("addr"));
  node->set_status(DHTNODE_LIVE);

  auto vnodes = flags.getInt("vnodes");
  for (size_t i = 0; i < vnodes; ++i) {
    auto token = Random::singleton()->sha1().toString();
    *node->add_sha1_tokens() = token;
  }

  cclient.updateClusterConfig(cluster);
}

int main(int argc, const char** argv) {
  stx::Application::init();
  stx::Application::logToStderr();

  stx::cli::FlagParser flags;

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

  /* command: cluster_add_node */
  auto cluster_add_node_cmd = cli.defineCommand("cluster-add-node");
  cluster_add_node_cmd->onCall(
      std::bind(&cmd_cluster_add_node, std::placeholders::_1));

  cluster_add_node_cmd->flags().defineFlag(
      "master",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "url",
      "<addr>");

  cluster_add_node_cmd->flags().defineFlag(
      "name",
      stx::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "node name",
      "<string>");

  cluster_add_node_cmd->flags().defineFlag(
      "addr",
      stx::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "node address",
      "<ip:port>");

  cluster_add_node_cmd->flags().defineFlag(
      "vnodes",
      stx::cli::FlagParser::T_INTEGER,
      true,
      NULL,
      NULL,
      "number of vnodes to assign",
      "<num>");

  cli.call(flags.getArgv());
  return 0;
}

