/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_ANALYTICSQUERY_H
#define _CM_ANALYTICSQUERY_H
#include <stx/stdtypes.h>
#include <stx/json/json.h>
#include "zbase/TrafficSegment.h"
#include "zbase/AnalyticsQueryParams.pb.h"
#include <logjoin/common.h>

using namespace stx;

namespace zbase {

struct AnalyticsQueryStatus : public RefCounted {
  AnalyticsQueryStatus();
  void update(size_t chunks_delta, size_t rows_delta);
  void onUpdate(Function<void ()> fn);
  double progress() const;
  String message() const;

  UnixTime start_time;
  size_t total_chunks;
  size_t completed_chunks;
  size_t rows_scanned;
  Function<void ()> on_update;
};

struct AnalyticsQuery {
  AnalyticsQuery() : status(new AnalyticsQueryStatus()) {}

  struct SubQueryParams {
    String query_type;
    Vector<Pair<String, String>> params;
  };

  String customer;
  String txid;
  uint64_t start_time;
  uint64_t end_time;
  Vector<TrafficSegmentParams> segments;
  Vector<SubQueryParams> queries;
  Vector<String> tables;

  RefPtr<AnalyticsQueryStatus> status;

  void build(const AnalyticsQuerySpec& spec);
  void rewrite();

  void toJSON(json::JSONOutputStream* json) const;
  void loadSegmentsJSON(const json::JSONObject& json);
};

} // namespace zbase

#endif
