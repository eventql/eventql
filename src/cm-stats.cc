/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stdlib.h>
#include "fnord/base/application.h"
#include "fnord/base/exceptionhandler.h"
#include "fnord/base/thread/eventloop.h"
#include "fnord/cli/flagparser.h"
#include "fnord/net/http/httpserver.h"
#include "fnord/net/http/httprouter.h"
#include "fnord/net/statsd/statsd.h"
#include "fnord/json/jsonrpc.h"
#include "fnord/json/jsonrpchttpadapter.h"
#include "fnord/service/metric/metricservice.h"
#include "fnord/service/metric/httpapiservlet.h"
#include "fnord/base/thread/threadpool.h"

using fnord::http::HTTPServer;
using fnord::http::HTTPRouter;
using fnord::json::JSONRPC;
using fnord::json::JSONRPCHTTPAdapter;
using fnord::metric_service::MetricService;
using fnord::thread::ThreadPool;

int main(int argc, const char** argv) {
  fnord::Application::init();
  fnord::Application::logToStderr();

  fnord::cli::FlagParser flags;

  flags.defineFlag(
      "rpc_http_port",
      fnord::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "8000",
      "Start the rpc http server on this port",
      "<port>");

  flags.defineFlag(
      "statsd_port",
      fnord::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "8192",
      "Start the statsd server on this port",
      "<port>");

  flags.defineFlag(
      "cmdata",
      fnord::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "clickmatcher app data dir",
      "<path>");

  flags.parseArgv(argc, argv);

  fnord::thread::ThreadPool thread_pool;
  fnord::thread::EventLoop evloop;

  /* set up cmdata */
  auto cmdata_path = flags.getString("cmdata");
  if (!fnord::FileUtil::isDirectory(cmdata_path)) {
    RAISEF(kIOError, "no such directory: $0", cmdata_path);
  }

  auto metrics_dir_path = fnord::FileUtil::joinPaths(cmdata_path, "metrics");
  fnord::FileUtil::mkdir_p(metrics_dir_path);

  auto metric_service = MetricService::newWithDiskBackend(
      metrics_dir_path,
      &thread_pool);

  HTTPRouter http_router;
  HTTPServer http_server(&http_router, &evloop);
  http_server.listen(flags.getInt("rpc_http_port"));

  fnord::metric_service::HTTPAPIServlet metrics_api(&metric_service);
  http_router.addRouteByPrefixMatch("/metrics", &metrics_api);

  fnord::statsd::StatsdServer statsd_server(&evloop, &evloop);
  statsd_server.onSample([&metric_service] (
      const std::string& key,
      double value,
      const std::vector<std::pair<std::string, std::string>>& labels) {
    metric_service.insertSample(key, value, labels);
  });
  statsd_server.listen(flags.getInt("statsd_port"));

  evloop.run();
  return 0;
}
