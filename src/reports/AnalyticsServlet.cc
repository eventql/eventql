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

  if (uri.path() == "/analytics/query_status") {
    fetchQueryStatus(uri, req, res);
    return;
  }

  if (uri.path() == "/analytics/query") {
    executeQuery(uri, req, res);
    return;
  }

  res->setStatus(fnord::http::kStatusNotFound);
  res->addBody("not found");
}

void AnalyticsServlet::fetchQueryStatus(
    const URI& uri,
    http::HTTPRequest* req,
    http::HTTPResponse* res) {
  const auto& params = uri.queryParams();

  String txid;
  if (!URI::getParam(params, "txid", &txid)) {
    res->addBody("error: missing ?txid=... parameter");
    res->setStatus(http::kStatusBadRequest);
    return;
  }

  auto status = engine_->queryStatus(txid);

  res->setStatus(http::kStatusOK);
  res->addHeader("Content-Type", "application/json; charset=utf-8");
  json::JSONOutputStream json(res->getBodyOutputStream());
  json.beginObject();
  json.addObjectEntry("status");
  if (status.isEmpty()) {
    json.addString("unknown");
  } else {
    auto progress =
        status.get().completed_chunks / (double) status.get().total_chunks;

    auto message = StringUtil::format(
        "Running... $0% ($1/$2)",
        progress * 100,
        status.get().completed_chunks,
        (double) status.get().total_chunks);

    json.addString("running");
    json.addComma();
    json.addObjectEntry("progress");
    json.addFloat(progress);
    json.addComma();
    json.addObjectEntry("message");
    json.addString(message);
  }

  json.endObject();
}

void AnalyticsServlet::executeQuery(
    const URI& uri,
    http::HTTPRequest* req,
    http::HTTPResponse* res) {
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

    if (p.first == "txid") {
      q.txid = p.second;
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
