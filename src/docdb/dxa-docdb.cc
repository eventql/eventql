/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "stx/stdtypes.h"
#include "stx/application.h"
#include "stx/logging.h"
#include "stx/cli/flagparser.h"
#include "stx/io/fileutil.h"
#include "stx/http/httprouter.h"
#include "stx/http/httpserver.h"
#include "stx/thread/eventloop.h"
#include "stx/thread/threadpool.h"
#include "docdb/DocumentDB.h"
#include "docdb/DocumentDBServlet.h"
#include "common/AnalyticsAuth.h"

using namespace stx;
using namespace cm;

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
      "7005",
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
  stx::thread::ThreadPool tpool;

  /* http */
  stx::http::HTTPRouter http_router;
  stx::http::HTTPServer http_server(&http_router, &ev);
  http_server.listen(flags.getInt("http_port"));

  /* auth */
  AnalyticsAuth auth;

  /* DocumentDB */
  DocumentDB docdb(flags.getString("datadir"));
  DocumentDBServlet docdb_servlet(&docdb, &auth);

  http_router.addRouteByPrefixMatch(
      "/",
      &docdb_servlet,
      &tpool);

  ev.run();
  stx::logInfo("dxa-docdb", "Exiting...");

  exit(0);
}

