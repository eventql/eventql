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
#include "stx/io/filerepository.h"
#include "stx/io/fileutil.h"
#include "stx/application.h"
#include "stx/logging.h"
#include "stx/random.h"
#include "stx/thread/eventloop.h"
#include "stx/thread/threadpool.h"
#include "stx/wallclock.h"
#include "stx/VFS.h"
#include "stx/rpc/ServerGroup.h"
#include "stx/rpc/RPC.h"
#include "stx/rpc/RPCClient.h"
#include "stx/cli/flagparser.h"
#include "stx/json/json.h"
#include "stx/json/jsonrpc.h"
#include "stx/http/httprouter.h"
#include "stx/http/httpserver.h"
#include "stx/http/VFSFileServlet.h"
#include "brokerd/FeedService.h"
#include "brokerd/RemoteFeedFactory.h"
#include "brokerd/RemoteFeedReader.h"
#include "stx/stats/statsdagent.h"
#include "sstable/SSTableServlet.h"
#include "zbase/util/mdb/MDB.h"
#include "zbase/util/mdb/MDBUtil.h"
#include "common.h"
#include "CustomerNamespace.h"
#include "ModelCache.h"
#include "AutoCompleteServlet.h"
#include "AutoCompleteModel.h"
#include "zbase/TermInfoTableSource.h"

using namespace zbase;
using namespace stx;

int main(int argc, const char** argv) {
  stx::Application::init();
  stx::Application::logToStderr();

  stx::cli::FlagParser flags;

  flags.defineFlag(
      "conf",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      "./conf",
      "conf directory",
      "<path>");

  flags.defineFlag(
      "http_port",
      stx::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "8000",
      "Start the public http server on this port",
      "<port>");

  flags.defineFlag(
      "datadir",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "datadir",
      "<path>");

  flags.defineFlag(
      "statsd",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      "127.0.0.1:8192",
      "Statsd addr",
      "<addr>");

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

  auto conf_path = flags.getString("conf");
  auto analyzer = RefPtr<fts::Analyzer>(new fts::Analyzer(conf_path));

  ModelCache models(flags.getString("datadir"));
  models.addModelFactory(
      "AutoCompleteModel", 
      [analyzer] (const String& filepath) -> RefCounted* {
        return new AutoCompleteModel(filepath, analyzer);
      });

  // preheat
  models.getModel("AutoCompleteModel", "termstats", "termstats-dawanda");

  zbase::AutoCompleteServlet acservlet(&models);

  /* start http server */
  stx::thread::EventLoop ev;
  stx::thread::ThreadPool tpool;
  stx::http::HTTPRouter http_router;
  http_router.addRouteByPrefixMatch("/autocomplete", &acservlet, &tpool);
  stx::http::HTTPServer http_server(&http_router, &ev);
  http_server.listen(flags.getInt("http_port"));

  /* stats reporting */
  stats::StatsdAgent statsd_agent(
      InetAddr::resolve(flags.getString("statsd")),
      10 * kMicrosPerSecond);

  statsd_agent.start();

  ev.run();
  return 0;
}

