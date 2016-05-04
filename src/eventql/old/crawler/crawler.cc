/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <eventql/util/logging.h>
#include <eventql/util/http/httprequest.h>
#include <eventql/util/json/json.h>
#include "crawler/crawler.h"
#include "crawler/crawlresult.h"

namespace zbase {

const char Crawler::kUserAgent[] = "cm-crawler (http://fnrd.net/)";

Crawler::Crawler(
    stx::feeds::RemoteFeedFactory feed_factory,
    size_t max_concurrency,
    stx::TaskScheduler* scheduler) :
    feed_cache_(feed_factory),
    max_concurrency_(max_concurrency),
    scheduler_(scheduler),
    http_(scheduler),
    in_flight_(0) {}

void Crawler::enqueue(const CrawlRequest& req) {
  std::unique_lock<std::mutex> lk(enqueue_lock_);

  while (in_flight_.load() > max_concurrency_) {
    wakeup_.waitForNextWakeup();
  }

  in_flight_++;
  lk.unlock();

  scheduler_->run([req, this] {
    auto http_req = stx::http::HTTPRequest::mkGet(req.url);
    http_req.setHeader("User-Agent", kUserAgent);

    enqueue(req, http_req);
  });
}

void Crawler::enqueue(
    const CrawlRequest& req,
    const stx::http::HTTPRequest http_req) {
  try {
    auto future = http_.executeRequest(http_req);
    scheduler_->runOnFirstWakeup(
        std::bind(&Crawler::requestReady, this, req, future),
        future.wakeup());
  } catch (const std::exception& e) {
    stx::logError("cm.crawler", e, "error while executing request");
    return;
  }
}

void Crawler::requestReady(
    CrawlRequest req,
    stx::Future<stx::http::HTTPResponse> res_future) {
  in_flight_--;
  wakeup_.wakeup();

  const auto& res = res_future.get();
  auto status = res.statusCode();

  /* follow redirects */
  if ((status == 301 || status == 302) && req.follow_redirects) {
    auto new_url = res.getHeader("Location");

    stx::logDebug(
        "cm.crawler",
        "following $1 redirect for: $0 => $2",
        req.url,
        res.statusCode(),
        new_url);

    auto http_req = stx::http::HTTPRequest::mkGet(new_url);
    http_req.setHeader("User-Agent", kUserAgent);

    in_flight_++;
    enqueue(req, http_req);

    return;
  }

  /* bail on error */
  if (status != 200) {
    stx::logError(
        "cm.crawler",
        "received non-200 status code for: $0 => $1, $2",
        req.url,
        res.statusCode(),
        res.body().toString());
  }

  CrawlResult result;
  result.url = req.url;
  result.userdata = req.userdata;

  auto feed = feed_cache_.getFeed(req.target_feed);
  auto future = feed->appendEntry(
      stx::StringUtil::format(
          "$0\n$1",
          stx::json::toJSONString(result),
          res.body().toString()));

  future.onFailure([] (const stx::Status& status) {
    stx::logError("cm.crawler", "error writing to feed: $0", status);
  });

  stx::logDebug("cm.crawler", "successfully crawled $0", req.url);
}

} // namespace zbase
