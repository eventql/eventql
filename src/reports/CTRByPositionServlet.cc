/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <thread>
#include "CTRByPositionServlet.h"
#include "CTRCounter.h"
#include "fnord-base/Language.h"
#include "fnord-base/logging.h"
#include "fnord-base/wallclock.h"
#include "fnord-base/io/fileutil.h"
#include "analytics/CTRByPositionRollup.h"
#include "analytics/TrafficSegment.h"

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

  cm::CTRByPositionRollupResult result;
  std::mutex mutex;
  size_t num_threads = 0;
  std::condition_variable cv;

  TrafficSegmentParams all_traffic {
      .key  = "all_traffic",
      .name = "All Traffic"
  };

  TrafficSegmentParams pl_traffic {
      .key  = "pl_traffic",
      .name = "PL Traffic"
  };

  pl_traffic.rules.emplace_back("query.language", TrafficSegmentOp::MATCHES, "pl");

  Vector<TrafficSegmentParams> segments;
  segments.emplace_back(all_traffic);
  segments.emplace_back(pl_traffic);

  for (uint64_t i = end_time; i >= start_time; i -= kMicrosPerHour * 4) {
    auto table_file = StringUtil::format(
        "/srv/cmdata/artifacts/$0_joined_sessions.$1.cstable",
        customer,
        i / (kMicrosPerHour * 4));

    if (!FileUtil::exists(table_file)) {
      fnord::logWarning(
          "cm.ctrbypositionservlet",
          "missing table: $0",
          table_file);

      continue;
    }

    std::unique_lock<std::mutex> lk(mutex);
    while (num_threads >= 8) {
      cv.wait(lk);
    }

    ++num_threads;
    auto t = std::thread([table_file, &mutex, &result, &num_threads, &cv, segments] () {
      cstable::CSTableReader reader(table_file);
      cm::AnalyticsTableScan aq;

      Vector<RefPtr<cm::TrafficSegment>> segs;
      for (const auto& s : segments) {
        segs.emplace_back(new TrafficSegment(s));
      }

      cm::CTRByPositionRollupResult r;
      cm::CTRByPositionRollup q(&aq, segs, &r);
      aq.scanTable(&reader);

      std::unique_lock<std::mutex> lk(mutex);
      result.merge(r);
      --num_threads;
      lk.unlock();
      cv.notify_one();
    });

    t.detach();
  }

  std::unique_lock<std::mutex> lk(mutex);
  while (num_threads > 0) {
    cv.wait(lk);
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

  json.addObjectEntry("segments");
  json.beginArray();
  for (int i = 0; i < segments.size(); ++i) {
    if (i > 0) json.addComma();
    segments[i].toJSON(&json);
  }
  json.endArray();
  json.addComma();

  json.addObjectEntry("results");
  result.toJSON(&json);
  json.endObject();
}

}
