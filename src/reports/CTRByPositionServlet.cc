/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "CTRByPositionServlet.h"
#include "CTRCounter.h"
#include "fnord-base/Language.h"
#include "fnord-base/wallclock.h"
#include "fnord-base/io/fileutil.h"
#include "analytics/CTRByPositionQuery.h"

using namespace fnord;

namespace cm {

CTRByPositionServlet::CTRByPositionServlet(VFS* vfs) : vfs_(vfs) {}

void CTRByPositionServlet::handleHTTPRequest(
    http::HTTPRequest* req,
    http::HTTPResponse* res) {
  URI uri(req->uri());
  const auto& params = uri.queryParams();

  auto end_time = WallClock::unixMicros();
  auto start_time = end_time - 30 * kMicrosPerDay;
  Set<String> test_groups;
  Set<String> device_types;

  /* arguments */
  String customer;
  if (!URI::getParam(params, "customer", &customer)) {
    res->addBody("error: missing ?customer=... parameter");
    res->setStatus(http::kStatusBadRequest);
    return;
  }

  String lang_str;
  if (!URI::getParam(params, "lang", &lang_str)) {
    res->addBody("error: missing ?lang=... parameter");
    res->setStatus(http::kStatusBadRequest);
    return;
  }

  for (const auto& p : params) {
    if (p.first == "from") {
      start_time = std::stoul(p.second) * kMicrosPerSecond;
      continue;
    }

    if (p.first == "until") {
      end_time = std::stoul(p.second) * kMicrosPerSecond;
      continue;
    }

    if (p.first == "test_group") {
      test_groups.emplace(p.second);
      continue;
    }

    if (p.first == "device_type") {
      device_types.emplace(p.second);
      continue;
    }

  }

  if (test_groups.size() == 0) {
    test_groups.emplace("all");
  }

  if (device_types.size() == 0) {
    device_types = Set<String> { "unknown", "desktop", "tablet", "phone" };
  }


  /* execute query*/
  auto t0 = WallClock::unixMicros();

  cm::CTRByPositionQueryResult result;
  for (uint64_t i = end_time; i >= start_time; i -= kMicrosPerHour * 4) {
    auto table_file = StringUtil::format(
        "/srv/cmdata/artifacts/$0_joined_sessions.$1.cstable",
        customer,
        i / (kMicrosPerHour * 4));

    cstable::CSTableReader reader(table_file);
    cm::AnalyticsQuery aq;
    cm::CTRByPositionQuery q(&aq, &result);
    aq.scanTable(&reader);
  }

  uint64_t total_views = 0;
  uint64_t total_clicks = 0;

  for (const auto& c : result.counters) {
    total_views += c.second.num_views;
    total_clicks += c.second.num_clicks;
  }

  auto t1 = WallClock::unixMicros();

  /* write response */
  res->setStatus(http::kStatusOK);
  res->addHeader("Content-Type", "application/json; charset=utf-8");
  json::JSONOutputStream json(res->getBodyOutputStream());

  json.beginObject();
  json.addObjectEntry("rows_scanned");
  json.addInteger(result.rows_scanned);
  json.addComma();
  json.addObjectEntry("execution_time_ms");
  json.addFloat((t1 - t0) / 1000.0f);
  json.addComma();
  json.addObjectEntry("results");
  json.beginArray();

  int n = 0;
  for (const auto& c : result.counters) {
    if (++n > 1) {
      json.addComma();
    }

    json.beginObject();
    json.addObjectEntry("position");
    json.addInteger(c.first);
    json.addComma();
    json.addObjectEntry("views");
    json.addInteger(c.second.num_views);
    json.addComma();
    json.addObjectEntry("clicks");
    json.addInteger(c.second.num_clicks);
    json.addComma();
    json.addObjectEntry("ctr");
    json.addFloat(c.second.num_clicks / (double) c.second.num_views);
    json.addComma();
    json.addObjectEntry("ctr_base");
    json.addFloat(c.second.num_clicks / (double) total_views);
    json.addComma();
    json.addObjectEntry("clickshare");
    json.addFloat(c.second.num_clicks / (double) total_clicks);
    json.endObject();
  }

  json.endArray();
  json.endObject();
}

}
