/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#ifndef _CM_ANALYTICSQUERYRESULT_H
#define _CM_ANALYTICSQUERYRESULT_H
#include <stx/stdtypes.h>
#include <stx/csv/CSVOutputStream.h>
#include <stx/json/json.h>
#include "zbase/TrafficSegment.h"
#include "zbase/AnalyticsQuery.h"

using namespace stx;

namespace zbase {

struct AnalyticsSubQuery;

struct AnalyticsQueryResult : public RefCounted {
  AnalyticsQueryResult(const AnalyticsQuery& query);

  struct SubQueryResult : public RefCounted {
    virtual ~SubQueryResult() {}
    virtual void merge(const SubQueryResult& other) = 0;
    virtual void toJSON(json::JSONOutputStream* json) const = 0;
    virtual void toCSV(CSVOutputStream* csv) const {};
    virtual void encode(util::BinaryMessageWriter* writer) const = 0;
    virtual void decode(util::BinaryMessageReader* reader) = 0;
    virtual void applyTimeRange(UnixTime from, UnixTime until) {};
  };

  AnalyticsQuery query;
  Vector<RefPtr<SubQueryResult>> results;
  Vector<RefPtr<AnalyticsSubQuery>> subqueries;
  double execution_time;
  uint64_t rows_scanned;
  uint64_t rows_missing;
  String error;

  void merge(const AnalyticsQueryResult& other);
  void toJSON(json::JSONOutputStream* json) const;
  void toCSV(CSVOutputStream* csv) const;
  void encode(util::BinaryMessageWriter* writer) const;
  void decode(util::BinaryMessageReader* reader);
};

struct AnalyticsSubQuery : public RefCounted {
  virtual ~AnalyticsSubQuery() {}
  virtual RefPtr<AnalyticsQueryResult::SubQueryResult> result() = 0;
  virtual void reduceResult(RefPtr<AnalyticsQueryResult::SubQueryResult> r) {}

  virtual size_t version() const {
    return 1;
  }

};

} // namespace zbase

#endif
