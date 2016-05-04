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
#include "brokerd/FeedService.h"
#include "brokerd/RemoteFeedFactory.h"
#include "brokerd/RemoteFeedReader.h"
#include "eventql/util/stats/statsdagent.h"
#include "eventql/infra/sstable/SSTableServlet.h"
#include "eventql/util/mdb/MDB.h"
#include "eventql/util/mdb/MDBUtil.h"
#include "common.h"
#include "CustomerNamespace.h"

#include "IndexServlet.h"

using namespace stx;

int main(int argc, const char** argv) {
  stx::Application::init();
  stx::Application::logToStderr();

  stx::cli::FlagParser flags;

  flags.defineFlag(
      "index",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "index dir",
      "<path>");

  flags.defineFlag(
      "conf",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      "./conf",
      "conf dir",
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

  auto index_path = flags.getString("index");
  auto conf_path = flags.getString("conf");

  /* open index */
  auto index_reader = zbase::IndexReader::openIndex(index_path);
  auto analyzer = RefPtr<fts::Analyzer>(new fts::Analyzer(conf_path));

  /* start http server */
  stx::thread::EventLoop ev;
  stx::http::HTTPRouter http_router;
  stx::http::HTTPServer http_server(&http_router, &ev);
  http_server.listen(flags.getInt("http_port"));

  /* index servlet */
  zbase::IndexServlet index_servlet(index_reader, analyzer);
  http_router.addRouteByPrefixMatch("/index", &index_servlet);

  ev.run();
  return 0;
}

