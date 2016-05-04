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
#include "eventql/util/rpc/ServerGroup.h"
#include "eventql/util/rpc/RPC.h"
#include "eventql/util/rpc/RPCClient.h"
#include "eventql/util/cli/flagparser.h"
#include "eventql/util/json/json.h"
#include "eventql/util/json/jsonrpc.h"
#include "eventql/util/http/httprouter.h"
#include "eventql/util/http/httpserver.h"
#include "eventql/util/http/VFSFileServlet.h"
#include "eventql/util/stats/statsdagent.h"
#include "eventql/util/mdb/MDB.h"
#include "eventql/util/mdb/MDBUtil.h"
#include "eventql/util/protobuf/msg.h"
#include "eventql/util/protobuf/MessageSchema.h"
#include "eventql/core/TSDBService.h"
#include "eventql/core/TSDBServlet.h"
#include "eventql/core/TSDBNodeConfig.pb.h"

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
  auto conf = msg::parseText<zbase::TSDBNodeConfig>(conf_data);

  /* start http server and worker pools */
  stx::thread::ThreadPool tpool;
  http::HTTPConnectionPool http(&ev);
  stx::http::HTTPRouter http_router;
  stx::http::HTTPServer http_server(&http_router, &ev);
  http_server.listen(flags.getInt("http_port"));

  zbase::PartitionMap pmap(dir);
  pmap.open();

  zbase::TSDBService tsdb_node(&pmap);

  zbase::TSDBServlet tsdb_servlet(&tsdb_node, "/tmp");
  http_router.addRouteByPrefixMatch("/tsdb", &tsdb_servlet, &tpool);

  ev.run();
  stx::logInfo("tsdb", "Exiting...");

  exit(0);
}

