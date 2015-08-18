/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "stx/wallclock.h"
#include "stx/protobuf/msg.h"
#include "zbase/AnalyticsQueryReducer.h"
#include "zbase/AnalyticsQueryParams.pb.h"
#include "zbase/reco_engine/RecoEngineStatsReport.h"

using namespace stx;

namespace cm {

RecoEngineStatsReport::RecoEngineStatsReport(
    const ReportParams& params) :
    params_(params) {}

RefPtr<VFSFile> RecoEngineStatsReport::computeBlob(dproc::TaskContext* context) {
  auto dep = context->getDependency(0)->getInstanceAs<AnalyticsQueryReducer>();
  auto res = dep->queryResult();

  BufferRef buffer(new Buffer());
  json::JSONOutputStream json(BufferOutputStream::fromBuffer(buffer.get()));
  res->results[0]->toJSON(&json);

  return buffer.get();
}

String RecoEngineStatsReport::contentType() const {
  return "application/json; charset=utf-8";
}

List<dproc::TaskDependency> RecoEngineStatsReport::dependencies() const {
  AnalyticsQuerySpec query;
  query.set_customer(params_.customer());
  query.set_start_time(params_.from_unixmicros());
  query.set_end_time(params_.until_unixmicros());

  auto subquery = query.add_subqueries();
  subquery->set_query("reco_engine.RecoEngineStatsBreakdownQuery");

  URI::ParamList params;
  URI::parseQueryString(params_.params(), &params);

  String dimensions;
  if (URI::getParam(params, "dimensions", &dimensions)) {
    auto p = subquery->add_params();
    p->set_key("dimensions");
    p->set_value(dimensions);
  }

  String window_size;
  if (URI::getParam(params, "window_size", &window_size)) {
    auto p = subquery->add_params();
    p->set_key("window_size");
    p->set_value(window_size);
  }

  return List<dproc::TaskDependency> {
    dproc::TaskDependency {
      .task_name = "AnalyticsQueryReducer",
      .params = *msg::encode(query)
    }
  };
}

}
