/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <thread>
#include "AnalyticsServlet.h"
#include "CTRCounter.h"
#include "fnord-base/Language.h"
#include "fnord-base/logging.h"
#include "fnord-base/wallclock.h"
#include "fnord-base/io/fileutil.h"
#include "analytics/CTRByPositionRollup.h"
#include "analytics/TrafficSegment.h"
#include "analytics/AnalyticsQuery.h"
#include "analytics/AnalyticsQueryEngine.h"
#include "analytics/AnalyticsQueryResult.h"

using namespace fnord;

namespace cm {

AnalyticsServlet::AnalyticsServlet(
    AnalyticsQueryEngine* engine) :
    engine_(engine) {}

void AnalyticsServlet::handleHTTPRequest(
    http::HTTPRequest* req,
    http::HTTPResponse* res) {
  URI uri(req->uri());
  const auto& params = uri.queryParams();

  AnalyticsQuery q;
  q.end_time = WallClock::unixMicros();
  q.start_time = q.end_time - 30 * kMicrosPerDay;

  if (!URI::getParam(params, "customer", &q.customer)) {
    res->addBody("error: missing ?customer=... parameter");
    res->setStatus(http::kStatusBadRequest);
    return;
  }

  for (const auto& p : params) {
    if (p.first == "from") {
      q.start_time = std::stoul(p.second) * kMicrosPerSecond;
      continue;
    }

    if (p.first == "until") {
      q.end_time = std::stoul(p.second) * kMicrosPerSecond;
      continue;
    }

    if (p.first == "query") {
      q.queries.emplace_back(AnalyticsQuery::SubQueryParams {
          .query_type = p.second });
      continue;
    }
  }

  if (q.segments.size() == 0) {
    q.segments.emplace_back(TrafficSegmentParams {
        .key  = "all_traffic",
        .name = "All Traffic" });
  }

  /* execute query */
  AnalyticsQueryResult result(q);
  engine_->executeQuery(&result);

  /* write response */
  res->setStatus(http::kStatusOK);
  res->addHeader("Content-Type", "application/json; charset=utf-8");
  json::JSONOutputStream json(res->getBodyOutputStream());
  result.toJSON(&json);
}

}
