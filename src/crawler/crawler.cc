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

const char Crawler::kUserAgent[] = "cm-crawler (http://fnrd.net/)";

Crawler::Crawler(
    size_t max_concurrency,
    fnord::thread::TaskScheduler* scheduler) :
    max_concurrency_(max_concurrency),
    scheduler_(scheduler),
    http_(scheduler),
    in_flight_(0) {}

void Crawler::enqueue(const CrawlRequest& req) {
  std::unique_lock<std::mutex> lk(enqueue_lock_);

  while (in_flight_.load() > max_concurrency_) {
    wakeup_.waitForNextWakeup();
  }

  auto http_req = fnord::http::HTTPRequest::mkGet(req.url);
  http_req.setHeader("User-Agent", kUserAgent);

  fnord::http::HTTPResponseFuture* res_future;
  try {
    auto f = http_.executeRequest(http_req);
    res_future = f.get();
    f.release();
  } catch (const std::exception& e) {
    fnord::log::Logger::get()->logException(
        fnord::log::kError,
        "[cm-crawlworker] error while executing request: $0",
        e);

    return;
  }

  fnord::iputs("enqueue called, in flight: $0 $1", in_flight_.load(), res_future);

  scheduler_->runOnFirstWakeup(
      std::bind(&Crawler::requestReady, this, req, res_future),
      res_future->onReady());

  in_flight_++;
}

void Crawler::requestReady(
    CrawlRequest req,
    fnord::http::HTTPResponseFuture* res_raw) {
  std::unique_ptr<fnord::http::HTTPResponseFuture> res_holder(res_raw);
  in_flight_--;
  wakeup_.wakeup();

  const auto& res = res_holder->get();
  if (res.statusCode() != 200) {
    fnord::log::Logger::get()->logf(
        fnord::log::kError,
        "[cm-crawlworker] received non-200 status code for: $0 => $1, $2",
        req.url,
        res.statusCode(),
        res.body().toString());
  }

  fnord::iputs("ready! $0", res.statusCode());
}

} // namespace cm
