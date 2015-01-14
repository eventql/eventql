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
#include "fnord/base/application.h"
#include "fnord/base/option.h"
#include "fnord/base/random.h"
#include "fnord/base/status.h"
#include "fnord/comm/lbgroup.h"
#include "fnord/comm/rpc.h"
#include "fnord/comm/queue.h"
#include "fnord/cli/flagparser.h"
#include "fnord/comm/rpcchannel.h"
#include "fnord/base/io/filerepository.h"
#include "fnord/base/io/fileutil.h"
#include "fnord/json/json.h"
#include "fnord/json/jsonrpc.h"
#include "fnord/json/jsonrpchttpchannel.h"
#include "fnord/net/http/httprouter.h"
#include "fnord/net/http/httpserver.h"
#include "fnord/net/redis/redisconnection.h"
#include "fnord/net/redis/redisqueue.h"
#include "fnord/base/thread/eventloop.h"
#include "fnord/base/thread/threadpool.h"
#include "fnord/service/logstream/logstreamservice.h"
#include "fnord/service/logstream/feedfactory.h"

#include "customernamespace.h"
#include "crawler/crawlrequest.h"
#include "crawler/crawler.h"

int main(int argc, const char** argv) {
  fnord::Application::init();
  fnord::Application::logToStderr();

  fnord::cli::FlagParser flags;

  flags.defineFlag(
      "feedserver_jsonrpc_url",
      fnord::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "feedserver jsonrpc url",
      "<url>");

  flags.defineFlag(
      "queue_redis_server",
      fnord::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "queue redis server addr",
      "<addr>");

  flags.defineFlag(
      "cm_env",
      fnord::cli::FlagParser::T_STRING,
      true,
      NULL,
      "dev",
      "cm env",
      "<env>");

  flags.defineFlag(
      "concurrency",
      fnord::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "100",
      "max number of http requests to run concurrently",
      "<env>");


  flags.parseArgv(argc, argv);

  /* start event loop */
  fnord::thread::EventLoop ev;

  auto evloop_thread = std::thread([&ev] {
    ev.run();
  });

  /* set up feedserver channel */
  fnord::comm::RoundRobinLBGroup feedserver_lbgroup;
  fnord::json::JSONRPCHTTPChannel feedserver_chan(
      &feedserver_lbgroup,
      &ev);

  feedserver_lbgroup.addServer(flags.getString("feedserver_jsonrpc_url"));
  fnord::logstream_service::LogStreamServiceFeedFactory feeds(&feedserver_chan);

  auto concurrency = flags.getInt("concurrency");
  fnord::logInfo(
      "cm.crawler",
      "Starting cm-crawlworker with concurrency=$0",
      concurrency);

  /* set up redis queue */
  auto redis_addr = fnord::net::InetAddr::resolve(
      flags.getString("queue_redis_server"));

  auto redis = fnord::redis::RedisConnection::connect(redis_addr, &ev);
  fnord::redis::RedisQueue queue("cm.crawler.workqueue", std::move(redis));

  /* start the crawler */
  cm::Crawler crawler(&feeds, concurrency, &ev);
  for (;;) {
    auto job = queue.leaseJob().waitAndGet();
    auto req = fnord::json::fromJSON<cm::CrawlRequest>(job.job_data);
    crawler.enqueue(req);
    queue.commitJob(job, fnord::Status::success());
  }

  evloop_thread.join();
  return 0;
}

