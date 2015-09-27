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
#include <stx/random.h>
#include <stx/uri.h>
#include <stx/thread/queue.h>
#include <brokerd/RemoteFeed.h>
#include <brokerd/RemoteFeedFactory.h>
#include "brokerd/RemoteFeedWriter.h"
#include <stx/http/httpservice.h>
#include "stx/stats/stats.h"

using namespace stx;

namespace zbase {
class CustomerNamespace;

class TrackerServlet : public stx::http::HTTPService {
public:
  static const int kMinPixelVersion = 5;

  explicit TrackerServlet(
      feeds::RemoteFeedWriter* tracker_log_feed);

  void handleHTTPRequest(
      stx::http::HTTPRequest* request,
      stx::http::HTTPResponse* response) override;

  void exportStats(const std::string& path_prefix);

protected:

  void pushEvent(const std::string& logline);

  feeds::RemoteFeedWriter* tracker_log_feed_;

  HashMap<String, CustomerNamespace*> vhosts_;
  Random rnd_;

  stats::Counter<uint64_t> stat_rpc_requests_total_;
  stats::Counter<uint64_t> stat_rpc_errors_total_;
  stats::Counter<uint64_t> stat_loglines_total_;
  stats::Counter<uint64_t> stat_loglines_versiontooold_;
  stats::Counter<uint64_t> stat_loglines_invalid_;
  stats::Counter<uint64_t> stat_loglines_written_success_;
  stats::Counter<uint64_t> stat_loglines_written_failure_;
  stats::Counter<uint64_t> stat_index_requests_total_;
  stats::Counter<uint64_t> stat_index_requests_written_success_;
  stats::Counter<uint64_t> stat_index_requests_written_failure_;
};

} // namespace zbase
#endif
