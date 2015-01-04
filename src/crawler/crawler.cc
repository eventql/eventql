/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <fnord/net/http/httprequest.h>
#include <fnord/json/json.h>
#include "crawler/crawler.h"
#include "crawler/crawlresult.h"

namespace cm {

const char Crawler::kUserAgent[] = "cm-crawler (http://fnrd.net/)";

Crawler::Crawler(
    fnord::comm::FeedFactory* feed_factory,
    size_t max_concurrency,
    fnord::thread::TaskScheduler* scheduler) :
    feed_cache_(feed_factory),
    max_concurrency_(max_concurrency),
    scheduler_(scheduler),
    http_(scheduler),
    in_flight_(0) {}

void Crawler::enqueue(const CrawlRequest& req) {
  auto http_req = fnord::http::HTTPRequest::mkGet(req.url);
  http_req.setHeader("User-Agent", kUserAgent);

  std::unique_lock<std::mutex> lk(enqueue_lock_);

  while (in_flight_.load() > max_concurrency_) {
    wakeup_.waitForNextWakeup();
  }

  enqueue(req, http_req);
}

void Crawler::enqueue(
    const CrawlRequest& req,
    const fnord::http::HTTPRequest http_req) {
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
  auto status = res.statusCode();

  /* follow redirects */
  if ((status == 301 || status == 302) && req.follow_redirects) {
    auto new_url = res.getHeader("Location");

    fnord::log::Logger::get()->logf(
        fnord::log::kDebug,
        "[cm-crawlworker] following $1 redirect for: $0 => $2",
        req.url,
        res.statusCode(),
        new_url);

    auto http_req = fnord::http::HTTPRequest::mkGet(new_url);
    http_req.setHeader("User-Agent", kUserAgent);

    std::unique_lock<std::mutex> lk(enqueue_lock_);
    enqueue(req, http_req);

    return;
  }

  /* bail on error */
  if (status != 200) {
    fnord::log::Logger::get()->logf(
        fnord::log::kError,
        "[cm-crawlworker] received non-200 status code for: $0 => $1, $2",
        req.url,
        res.statusCode(),
        res.body().toString());
  }

  CrawlResult result;
  result.url = req.url;
  result.userdata = req.userdata;

  auto feed = feed_cache_.getFeed(req.target_feed);
  feed->append(
      fnord::StringUtil::format(
          "$0\n$1",
          fnord::json::toJSONString(result),
          res.body().toString()));

  fnord::log::Logger::get()->logf(
      fnord::log::kDebug,
      "[cm-crawlworker] successfully crawled $0",
      req.url);
}

} // namespace cm
