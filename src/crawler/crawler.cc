/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <fnord-base/logging.h>
#include <fnord-http/httprequest.h>
#include <fnord-json/json.h>
#include "crawler/crawler.h"
#include "crawler/crawlresult.h"

namespace cm {

const char Crawler::kUserAgent[] = "cm-crawler (http://fnrd.net/)";

Crawler::Crawler(
    fnord::feeds::RemoteFeedFactory feed_factory,
    size_t max_concurrency,
    fnord::TaskScheduler* scheduler) :
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
    auto http_req = fnord::http::HTTPRequest::mkGet(req.url);
    http_req.setHeader("User-Agent", kUserAgent);

    enqueue(req, http_req);
  });
}

void Crawler::enqueue(
    const CrawlRequest& req,
    const fnord::http::HTTPRequest http_req) {
  try {
    auto future = http_.executeRequest(http_req);
    scheduler_->runOnFirstWakeup(
        std::bind(&Crawler::requestReady, this, req, future),
        future.wakeup());
  } catch (const std::exception& e) {
    fnord::logError("cm.crawler", e, "error while executing request");
    return;
  }
}

void Crawler::requestReady(
    CrawlRequest req,
    fnord::Future<fnord::http::HTTPResponse> res_future) {
  in_flight_--;
  wakeup_.wakeup();

  const auto& res = res_future.get();
  auto status = res.statusCode();

  /* follow redirects */
  if ((status == 301 || status == 302) && req.follow_redirects) {
    auto new_url = res.getHeader("Location");

    fnord::logDebug(
        "cm.crawler",
        "following $1 redirect for: $0 => $2",
        req.url,
        res.statusCode(),
        new_url);

    auto http_req = fnord::http::HTTPRequest::mkGet(new_url);
    http_req.setHeader("User-Agent", kUserAgent);

    in_flight_++;
    enqueue(req, http_req);

    return;
  }

  /* bail on error */
  if (status != 200) {
    fnord::logError(
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
      fnord::StringUtil::format(
          "$0\n$1",
          fnord::json::toJSONString(result),
          res.body().toString()));

  future.onFailure([] (const fnord::Status& status) {
    fnord::logError("cm.crawler", "error writing to feed: $0", status);
  });

  fnord::logDebug("cm.crawler", "successfully crawled $0", req.url);
}

} // namespace cm
