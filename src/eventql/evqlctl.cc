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
#include "eventql/ConfigDirectory.h"
#include "eventql/infra/sstable/sstablereader.h"
#include "eventql/core/TimeWindowPartitioner.h"
#include "eventql/core/TSDBClient.h"
#include "eventql/sql/qtree/SequentialScanNode.h"
#include "eventql/sql/qtree/ColumnReferenceNode.h"
#include "eventql/sql/qtree/CallExpressionNode.h"
#include "eventql/sql/qtree/GroupByNode.h"
#include "eventql/sql/qtree/UnionNode.h"
#include "eventql/sql/runtime/queryplanbuilder.h"
#include "eventql/sql/CSTableScan.h"
#include "eventql/sql/CSTableScanProvider.h"
#include "eventql/sql/runtime/defaultruntime.h"
#include "eventql/sql/runtime/tablerepository.h"

using namespace util;
using namespace eventql;

util::thread::EventLoop ev;

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
  util::Application::init();
  util::Application::logToStderr();

  util::cli::FlagParser flags;

  flags.defineFlag(
      "loglevel",
      util::cli::FlagParser::T_STRING,
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
      util::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "node name",
      "<string>");

  cluster_add_node_cmd->flags().defineFlag(
      "addr",
      util::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "node address",
      "<ip:port>");

  cluster_add_node_cmd->flags().defineFlag(
      "vnodes",
      util::cli::FlagParser::T_INTEGER,
      true,
      NULL,
      NULL,
      "number of vnodes to assign",
      "<num>");

  cli.call(flags.getArgv());
  return 0;
}

