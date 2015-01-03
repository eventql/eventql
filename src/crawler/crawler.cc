/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <fnord/net/http/httprequest.h>
#include "crawler/crawler.h"

namespace cm {

Crawler::Crawler(
    size_t max_concurrency,
    fnord::thread::TaskScheduler* scheduler) :
    max_concurrency_(max_concurrency),
    scheduler_(scheduler),
    http_(scheduler),
    in_flight_(0) {}

void Crawler::enqueue(const CrawlRequest& req) {
  while (in_flight_.load() > max_concurrency_) {
    wakeup_.waitForNextWakeup();
  }

  auto http_req = fnord::http::HTTPRequest::mkGet(req.url);
  auto res_future = http_.executeRequest(http_req);
  auto res_future_ptr = res_future.get();
  res_future.release();

  scheduler_->runOnFirstWakeup(
      std::bind(&Crawler::requestReady, this, req, res_future_ptr),
      res_future_ptr->onReady());

  in_flight_++;
}

void Crawler::requestReady(
    CrawlRequest req,
    fnord::http::HTTPResponseFuture* res_raw) {
  std::unique_ptr<fnord::http::HTTPResponseFuture> res(res_raw);
  in_flight_--;
  wakeup_.wakeup();

  fnord::iputs("ready!", 1);
}

} // namespace cm
