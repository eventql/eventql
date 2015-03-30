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
#include <fnord-base/random.h>
#include <fnord-base/uri.h>
#include <fnord-feeds/RemoteFeed.h>
#include <fnord-feeds/RemoteFeedFactory.h>
#include "fnord-feeds/RemoteFeedWriter.h"
#include <fnord-http/httpservice.h>
#include "fnord-base/stats/stats.h"
#include "common.h"
#include "IndexChangeRequest.h"

using namespace fnord;

namespace cm {
class CustomerNamespace;

class CMFrontend : public fnord::http::HTTPService {
public:
  static const int kMinPixelVersion = 5;

  explicit CMFrontend(feeds::RemoteFeedWriter* tracker_log_feed);

  void handleHTTPRequest(
      fnord::http::HTTPRequest* request,
      fnord::http::HTTPResponse* response) override;

  void addCustomer(
      CustomerNamespace* customer,
      RefPtr<feeds::RemoteFeedWriter> index_request_feed_writer);

  void exportStats(const std::string& path_prefix);

protected:

  void dispatchRPC(json::JSONRPCRequest* req, json::JSONRPCResponse* res);

  void recordIndexChangeRequest(const IndexChangeRequest& index_request);

  void track(CustomerNamespace* customer, const fnord::URI& uri);
  void recordLogLine(CustomerNamespace* customer, const std::string& logline);

  feeds::RemoteFeedWriter* tracker_log_feed_;

  HashMap<String, CustomerNamespace*> vhosts_;
  HashMap<String, RefPtr<feeds::RemoteFeedWriter>> index_request_feeds_;
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

} // namespace cm
#endif
