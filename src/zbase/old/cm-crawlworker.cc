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
#include "stx/application.h"
#include "stx/option.h"
#include "stx/random.h"
#include "stx/status.h"
#include "stx/rpc/ServerGroup.h"
#include "stx/rpc/RPC.h"
#include "stx/comm/queue.h"
#include "stx/cli/flagparser.h"
#include "stx/comm/rpcchannel.h"
#include "stx/io/filerepository.h"
#include "stx/io/fileutil.h"
#include "stx/json/json.h"
#include "stx/json/jsonrpc.h"
#include "stx/json/jsonrpchttpchannel.h"
#include "stx/http/httprouter.h"
#include "stx/http/httpserver.h"
#include "stx/net/redis/redisconnection.h"
#include "stx/net/redis/redisqueue.h"
#include "stx/thread/eventloop.h"
#include "stx/thread/threadpool.h"
#include "brokerd/FeedService.h"
#include "brokerd/RemoteFeedFactory.h"

#include "CustomerNamespace.h"
#include "crawler/crawlrequest.h"
#include "crawler/crawler.h"

int main(int argc, const char** argv) {
  stx::Application::init();
  stx::Application::logToStderr();

  stx::cli::FlagParser flags;

  flags.defineFlag(
      "feedserver_jsonrpc_url",
      stx::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "feedserver jsonrpc url",
      "<url>");

  flags.defineFlag(
      "queue_redis_server",
      stx::cli::FlagParser::T_STRING,
      true,
      NULL,
      NULL,
      "queue redis server addr",
      "<addr>");

  flags.defineFlag(
      "cmenv",
      stx::cli::FlagParser::T_STRING,
      true,
      NULL,
      "dev",
      "cm env",
      "<env>");

  flags.defineFlag(
      "concurrency",
      stx::cli::FlagParser::T_INTEGER,
      false,
      NULL,
      "100",
      "max number of http requests to run concurrently",
      "<env>");


  flags.parseArgv(argc, argv);

  /* start event loop */
  stx::thread::EventLoop ev;

  auto evloop_thread = std::thread([&ev] {
    ev.run();
  });

  /* set up feedserver channel */
  stx::comm::RoundRobinServerGroup feedserver_lbgroup;
  stx::json::JSONRPCHTTPChannel feedserver_chan(
      &feedserver_lbgroup,
      &ev);

  feedserver_lbgroup.addServer(flags.getString("feedserver_jsonrpc_url"));
  stx::feeds::RemoteFeedFactory feeds(&feedserver_chan);

  auto concurrency = flags.getInt("concurrency");
  stx::logInfo(
      "cm.crawler",
      "Starting cm-crawlworker with concurrency=$0",
      concurrency);

  /* set up redis queue */
  auto redis_addr = stx::InetAddr::resolve(
      flags.getString("queue_redis_server"));

  auto redis = stx::redis::RedisConnection::connect(redis_addr, &ev);
  stx::redis::RedisQueue queue("cm.crawler.workqueue", std::move(redis));

  /* start the crawler */
  zbase::Crawler crawler(&feeds, concurrency, &ev);
  for (;;) {
    auto job = queue.leaseJob().waitAndGet();
    auto req = stx::json::fromJSON<zbase::CrawlRequest>(job.job_data);
    crawler.enqueue(req);
    queue.commitJob(job, stx::Status::success());
  }

  evloop_thread.join();
  return 0;
}

