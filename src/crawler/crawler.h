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
#include <fnord/base/random.h>
#include <fnord/base/uri.h>
#include <fnord/comm/feed.h>
#include <fnord/thread/taskscheduler.h>
#include <fnord/net/http/httpconnectionpool.h>
#include "crawler/crawlrequest.h"

namespace cm {

class Crawler {
public:
  static const char kUserAgent[];

  explicit Crawler(
      size_t max_concurrency,
      fnord::thread::TaskScheduler* scheduler);

  void enqueue(const CrawlRequest& req);

protected:
  void requestReady(CrawlRequest req, fnord::http::HTTPResponseFuture* res);

  size_t max_concurrency_;
  fnord::thread::TaskScheduler* scheduler_;
  fnord::thread::Wakeup wakeup_;
  fnord::http::HTTPConnectionPool http_;
  std::atomic<size_t> in_flight_;
  std::mutex enqueue_lock_;
};

} // namespace cm
#endif
