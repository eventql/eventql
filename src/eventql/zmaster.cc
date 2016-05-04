/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "eventql/util/stdtypes.h"
#include "eventql/util/application.h"
#include "eventql/util/logging.h"
#include "eventql/util/cli/flagparser.h"
#include "eventql/util/io/fileutil.h"
#include "eventql/util/http/httprouter.h"
#include "eventql/util/http/httpserver.h"
#include "eventql/util/thread/eventloop.h"
#include "eventql/util/thread/threadpool.h"
#include "eventql/master/ConfigDirectoryMaster.h"
#include "eventql/master/MasterServlet.h"

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
      "9175",
      "Start the public http server on this port",
      "<port>");

  flags.defineFlag(
      "datadir",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "datadir path",
      "<path>");

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

  /* thread pools */
  stx::thread::ThreadPool tpool(thread::ThreadPoolOptions{});

  /* http */
  stx::http::HTTPRouter http_router;
  stx::http::HTTPServer http_server(&http_router, &ev);
  http_server.listen(flags.getInt("http_port"));

  /* customer directory */
  auto cdb_dir = FileUtil::joinPaths(
      flags.getString("datadir"),
      "master");

  if (!FileUtil::exists(cdb_dir)) {
    FileUtil::mkdir_p(cdb_dir);
  }

  ConfigDirectoryMaster customer_dir(cdb_dir);

  MasterServlet master_servlet(&customer_dir);
  http_router.addRouteByPrefixMatch(
      "/analytics/master",
      &master_servlet,
      &tpool);

  ev.run();
  stx::logInfo("dxa-master", "Exiting...");

  exit(0);
}

