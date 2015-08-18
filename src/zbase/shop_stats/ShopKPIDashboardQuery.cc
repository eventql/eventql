/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "stx/wallclock.h"
#include "stx/logging.h"
#include "zbase/AnalyticsTableScanPlanner.h"
#include "ShopKPIDashboardQuery.h"
#include "ShopStats.h"

using namespace stx;

namespace cm {

ShopKPIDashboardQuery::ShopKPIDashboardQuery(
    const ReportParams& params,
    tsdb::TSDBService* tsdb) :
    source_(params, tsdb) {}

RefPtr<VFSFile> ShopKPIDashboardQuery::computeBlob(dproc::TaskContext* context) {
  Vector<Vector<csql::SValue>> timeseries;
  source_.forEach([this, &timeseries] (const Vector<csql::SValue>& row) -> bool {
    timeseries.emplace_back(row);
    return true;
  });

  source_.read(context);

  BufferRef buffer(new Buffer());
  json::JSONOutputStream json(BufferOutputStream::fromBuffer(buffer.get()));
  json.beginObject();

  auto aggr = source_.aggregates();
  json.addObjectEntry("aggregates");
  Vector<csql::SValue> aggr_row;
  ShopStats::toRow(aggr, &aggr_row);
  rowToJSON(ShopStats::columns(aggr), aggr_row, &json);
  json.addComma();

  auto cols = source_.columns();
  json.addObjectEntry("timeseries");
  json.beginArray();
  for (int i = 0; i < timeseries.size(); ++i) {
    if (i > 0) json.addComma();
    rowToJSON(cols, timeseries[i], &json);
  }

  json.endArray();
  json.endObject();

  return buffer.get();
}

List<dproc::TaskDependency> ShopKPIDashboardQuery::dependencies() const {
  return source_.dependencies();
}

} // namespace cm

