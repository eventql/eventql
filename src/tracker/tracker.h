/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_TRACKER_H
#define _CM_TRACKER_H
#include <mutex>
#include <stdlib.h>
#include <set>
#include <string>
#include <unordered_map>
#include <fnord/base/random.h>
#include <fnord/base/uri.h>
#include <fnord/comm/feed.h>
#include <fnord/net/http/httpservice.h>
#include "fnord/stats/stats.h"

namespace cm {
class CustomerNamespace;

class Tracker : public fnord::http::HTTPService {
public:
  static const int kMinPixelVersion = 4;

  explicit Tracker(fnord::comm::FeedFactory* feed_factory);

  static bool isReservedParam(const std::string param);

  void handleHTTPRequest(
      fnord::http::HTTPRequest* request,
      fnord::http::HTTPResponse* response) override;

  void addCustomer(CustomerNamespace* customer);

protected:

  void track(CustomerNamespace* customer, const fnord::URI& uri);
  void recordLogLine(CustomerNamespace* customer, const std::string& logline);

  std::unique_ptr<fnord::comm::Feed> feed_;
  std::unordered_map<std::string, std::unique_ptr<fnord::comm::Feed>> feeds_;
  std::mutex feeds_mutex_;

  std::unordered_map<std::string, CustomerNamespace*> vhosts_;
  fnord::Random rnd_;

  fnord::stats::Counter<uint64_t> stat_loglines_total_;
  fnord::stats::Counter<uint64_t> stat_loglines_versiontooold_;
  fnord::stats::Counter<uint64_t> stat_loglines_invalid_;
  fnord::stats::Counter<uint64_t> stat_loglines_written_success_;
  fnord::stats::Counter<uint64_t> stat_loglines_written_failure_;
};

} // namespace cm
#endif
