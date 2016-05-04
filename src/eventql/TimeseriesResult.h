/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_TIMESERIESRESULT_H
#define _CM_TIMESERIESRESULT_H
#include <eventql/util/stdtypes.h>
#include <eventql/util/json/json.h>
#include "eventql/infra/cstable/CSTableReader.h"
#include "eventql/CTRCounter.h"
#include "eventql/AnalyticsTableScan.h"
#include "eventql/TrafficSegment.h"
#include "eventql/AnalyticsQueryResult.h"
#include "eventql/GroupResult.h"

using namespace stx;

namespace zbase {

template <typename PointType>
struct TimeseriesResult : public AnalyticsQueryResult::SubQueryResult {
  void merge(const AnalyticsQueryResult::SubQueryResult& other);
  void toJSON(json::JSONOutputStream* json) const override;
  void encode(util::BinaryMessageWriter* writer) const override;
  void decode(util::BinaryMessageReader* reader) override;
  void applyTimeRange(UnixTime from, UnixTime until) override;

  Vector<HashMap<uint64_t, PointType>> series;
  Vector<String> segment_keys;
};

template <typename PointType>
struct TimeseriesDrilldownResult : public AnalyticsQueryResult::SubQueryResult {
  void merge(const AnalyticsQueryResult::SubQueryResult& other);
  void toJSON(json::JSONOutputStream* json) const override;
  void encode(util::BinaryMessageWriter* writer) const override;
  void decode(util::BinaryMessageReader* reader) override;
  void applyTimeRange(UnixTime from, UnixTime until) override;

  TimeseriesResult<PointType> timeseries;
  GroupResult<String, PointType> drilldown;
};

template <typename PointType>
struct TimeseriesBreakdownResult : public AnalyticsQueryResult::SubQueryResult {
  void merge(const AnalyticsQueryResult::SubQueryResult& other);
  void toJSON(json::JSONOutputStream* json) const override;
  void encode(util::BinaryMessageWriter* writer) const override;
  void decode(util::BinaryMessageReader* reader) override;
  void applyTimeRange(UnixTime from, UnixTime until) override;

  Vector<String> dimensions;
  HashMap<uint64_t, HashMap<String, PointType>> timeseries;
};



} // namespace zbase

#include "TimeseriesResult_impl.h"
#endif
