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

#include "eventql/eventql.h"

std::atomic<bool> shutdown_sig;
util::thread::EventLoop ev;

int main(int argc, const char** argv) {
  util::Application::init();
  util::Application::logToStderr();

  cli::FlagParser flags;

  flags.defineFlag(
      "http_port",
      cli::FlagParser::T_INTEGER,
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
      cli::FlagParser::T_STRING,
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
  auto conf = msg::parseText<eventql::TSDBNodeConfig>(conf_data);

  /* start http server and worker pools */
  util::thread::ThreadPool tpool;
  http::HTTPConnectionPool http(&ev);
  http::HTTPRouter http_router;
  http::HTTPServer http_server(&http_router, &ev);
  http_server.listen(flags.getInt("http_port"));

  eventql::PartitionMap pmap(dir);
  pmap.open();

  eventql::TSDBService tsdb_node(&pmap);

  eventql::TSDBServlet tsdb_servlet(&tsdb_node, "/tmp");
  http_router.addRouteByPrefixMatch("/tsdb", &tsdb_servlet, &tpool);

  ev.run();
  logInfo("tsdb", "Exiting...");

  exit(0);
}

