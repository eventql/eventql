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
#include "ShopProductsDashboardQuery.h"
#include "ShopStats.h"

using namespace stx;

namespace zbase {

ShopProductsDashboardQuery::ShopProductsDashboardQuery(
    const ReportParams& params,
    zbase::TSDBService* tsdb) :
    source_(params, tsdb) {}

RefPtr<VFSFile> ShopProductsDashboardQuery::computeBlob(dproc::TaskContext* context) {
  Vector<Vector<csql::SValue>> products;
  source_.forEach([this, &products] (const Vector<csql::SValue>& row) -> bool {
    products.emplace_back(row);
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

  json.addObjectEntry("products");
  json.beginArray();
  auto cols = source_.columns();
  for (int i = 0; i < products.size(); ++i) {
    if (i > 0) json.addComma();
    rowToJSON(cols, products[i], &json);
  }

  json.endArray();
  json.endObject();

  return buffer.get();
}

List<dproc::TaskDependency> ShopProductsDashboardQuery::dependencies() const {
  return source_.dependencies();
}

} // namespace zbase

