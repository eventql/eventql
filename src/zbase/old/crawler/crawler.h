/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_CRAWLER_H
#define _CM_CRAWLER_H
#include <mutex>
#include <stdlib.h>
#include <set>
#include <string>
#include <unordered_map>
#include <stx/random.h>
#include <stx/uri.h>
#include <stx/thread/taskscheduler.h>
#include <brokerd/RemoteFeed.h>
#include <stx/http/httpconnectionpool.h>
#include "crawler/crawlrequest.h"

namespace zbase {

class Crawler {
public:
  static const char kUserAgent[];

  explicit Crawler(
      stx::feeds::RemoteFeedFactory feed_factory,
      size_t max_concurrency,
      stx::TaskScheduler* scheduler);

  void enqueue(const CrawlRequest& req);

protected:
  void enqueue(
      const CrawlRequest& req,
      const stx::http::HTTPRequest http_req);

  void requestReady(
      CrawlRequest req,
      stx::Future<stx::http::HTTPResponse> res);

  stx::feeds::RemoteFeedCache feed_cache_;
  size_t max_concurrency_;
  stx::TaskScheduler* scheduler_;
  stx::Wakeup wakeup_;
  stx::http::HTTPConnectionPool http_;
  std::atomic<size_t> in_flight_;
  std::mutex enqueue_lock_;
};

} // namespace zbase
#endif
