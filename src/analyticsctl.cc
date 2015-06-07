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
#include "fnord-base/cli/flagparser.h"
#include "fnord-json/json.h"
#include "fnord-json/jsonrpc.h"
#include "fnord-http/httprouter.h"
#include "fnord-http/httpserver.h"
#include "fnord-http/VFSFileServlet.h"
#include "fnord-dproc/LocalScheduler.h"
#include "fnord-dproc/DispatchService.h"
#include "common.h"
#include "schemas.h"
#include "CustomerNamespace.h"
#include "analytics/AnalyticsServlet.h"
#include "analytics/FeedExportApp.h"
#include "analytics/ShopStatsServlet.h"
#include "analytics/AnalyticsApp.h"

using namespace fnord;

fnord::thread::EventLoop ev;

void cmd_rebuild(const cli::FlagParser& flags) {
  fnord::iputs("monitor", 1);

}

int main(int argc, const char** argv) {
  fnord::Application::init();
  fnord::Application::logToStderr();

  cli::CLI cli;

  /* command: monitor */
  auto monitor_cmd = cli.defineCommand("monitor");
  monitor_cmd->onCall(std::bind(&cmd_monitor, std::placeholders::_1));

  /* command: export */
  auto export_cmd = cli.defineCommand("export");
  export_cmd->onCall(std::bind(&cmd_export, std::placeholders::_1));

  export_cmd->flags().defineFlag(
      "topic",
      fnord::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "topic",
      "<topic>");

  export_cmd->flags().defineFlag(
      "datadir",
      fnord::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "output file path",
      "<path>");

  export_cmd->flags().defineFlag(
      "server",
      fnord::cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "backend servers",
      "<host>");


  cli.call(flags.getArgv());
  return 0;
}

