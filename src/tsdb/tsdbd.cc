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
#include "fnord/io/filerepository.h"
#include "fnord/io/fileutil.h"
#include "fnord/application.h"
#include "fnord/logging.h"
#include "fnord/random.h"
#include "fnord/thread/eventloop.h"
#include "fnord/thread/threadpool.h"
#include "fnord/thread/FixedSizeThreadPool.h"
#include "fnord/wallclock.h"
#include "fnord/VFS.h"
#include "fnord/rpc/ServerGroup.h"
#include "fnord/rpc/RPC.h"
#include "fnord/rpc/RPCClient.h"
#include "fnord/cli/flagparser.h"
#include "fnord/json/json.h"
#include "fnord/json/jsonrpc.h"
#include "fnord/http/httprouter.h"
#include "fnord/http/httpserver.h"
#include "fnord/http/VFSFileServlet.h"
#include "fnord/stats/statsdagent.h"
#include "fnord/mdb/MDB.h"
#include "fnord/mdb/MDBUtil.h"
#include "fnord/protobuf/MessageSchema.h"
#include "tsdb/TSDBNode.h"
#include "tsdb/TSDBServlet.h"

using namespace fnord;

std::atomic<bool> shutdown_sig;
fnord::thread::EventLoop ev;

int main(int argc, const char** argv) {
  fnord::Application::init();
  fnord::Application::logToStderr();

  fnord::cli::FlagParser flags;

  flags.defineFlag(
      "http_port",
      fnord::cli::FlagParser::T_INTEGER,
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
      fnord::cli::FlagParser::T_STRING,
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

  /* start http server and worker pools */
  fnord::thread::ThreadPool tpool;
  http::HTTPConnectionPool http(&ev);
  fnord::http::HTTPRouter http_router;
  fnord::http::HTTPServer http_server(&http_router, &ev);
  http_server.listen(flags.getInt("http_port"));

  auto repl_scheme = mkRef(new dproc::FixedReplicationScheme());
  for (const auto& r : repl_targets) {
    repl_scheme->addHost(r);
  }

  tsdb::TSDBNode tsdb_node(dir, repl_scheme.get(), &http);

  tsdb::TSDBServlet tsdb_servlet(&tsdb_node);
  http_router.addRouteByPrefixMatch("/tsdb", &tsdb_servlet, &tpool);

  tsdb_node.start();
  ev.run();

  tsdb_node.stop();
  fnord::logInfo("tsdb", "Exiting...");

  exit(0);
}

