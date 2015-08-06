/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
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
#include "stx/thread/FixedSizeThreadPool.h"
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
#include "stx/stats/statsdagent.h"
#include "stx/mdb/MDB.h"
#include "stx/mdb/MDBUtil.h"
#include "stx/protobuf/msg.h"
#include "stx/protobuf/MessageSchema.h"
#include "tsdb/TSDBService.h"
#include "tsdb/TSDBServlet.h"
#include "tsdb/TSDBNodeConfig.pb.h"

using namespace stx;

std::atomic<bool> shutdown_sig;
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
      "8000",
      "Start the public http server on this port",
      "<port>");

  flags.defineFlag(
      "conf",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "conf file path",
      "<conf>");

  flags.defineFlag(
      "datadir",
      cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "datadir path",
      "<path>");

  flags.defineFlag(
      "replicate_to",
      cli::FlagParser::T_STRING,
      false,
      NULL,
      NULL,
      "url",
      "<url>");

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

  /* args */
  auto dir = flags.getString("datadir");
  auto repl_targets = flags.getStrings("replicate_to");

  /* conf */
  auto conf_data = FileUtil::read(flags.getString("conf"));
  auto conf = msg::parseText<tsdb::TSDBNodeConfig>(conf_data);

  /* start http server and worker pools */
  stx::thread::ThreadPool tpool;
  http::HTTPConnectionPool http(&ev);
  stx::http::HTTPRouter http_router;
  stx::http::HTTPServer http_server(&http_router, &ev);
  http_server.listen(flags.getInt("http_port"));

  tsdb::PartitionMap pmap(dir);
  pmap.open();

  tsdb::TSDBService tsdb_node(&pmap);

  tsdb::TSDBServlet tsdb_servlet(&tsdb_node, "/tmp");
  http_router.addRouteByPrefixMatch("/tsdb", &tsdb_servlet, &tpool);

  ev.run();
  stx::logInfo("tsdb", "Exiting...");

  exit(0);
}

