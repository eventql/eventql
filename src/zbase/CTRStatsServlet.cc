/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "CTRStatsServlet.h"
#include "zbase/CTRCounter.h"
#include "stx/Language.h"
#include "stx/wallclock.h"
#include "stx/io/fileutil.h"
#include "sstable/sstablereader.h"
#include "sstable/SSTableScan.h"

using namespace stx;

namespace zbase {

CTRStatsServlet::CTRStatsServlet(VFS* vfs) : vfs_(vfs) {}

void CTRStatsServlet::handleHTTPRequest(
    http::HTTPRequest* req,
    http::HTTPResponse* res) {
  URI uri(req->uri());
  const auto& params = uri.queryParams();

  Set<String> test_groups;
  Set<String> device_types;
  Set<String> pages;
  auto end_time = WallClock::unixMicros();
  auto start_time = end_time - 30 * kMicrosPerDay;

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

    if (p.first == "page_type") {
      pages.emplace(p.second);
      continue;
    }

  }

  if (test_groups.size() == 0) {
    test_groups.emplace("all");
  }

  if (device_types.size() == 0) {
    device_types = Set<String> { "unknown", "desktop", "tablet", "phone" };
  }

  /* prepare prefix filters for scanning */
  String scan_common_prefix = lang_str + "~";
  if (test_groups.size() == 1) {
    scan_common_prefix += *test_groups.begin() + "~";

    if (device_types.size() == 1) {
      scan_common_prefix += *device_types.begin() + "~";

      if (pages.size() == 1) {
        scan_common_prefix += *pages.begin();
      }
    }
  }

  /* scan input tables */
  HashMap<uint64_t, CTRCounterData> counters;
  CTRCounterData aggr_counter;
  for (uint64_t i = end_time; i >= start_time; i -= kMicrosPerDay) {
    auto& ctr_ref = counters[i];
    auto tbl = StringUtil::format(
        "$0_ctr_stats_daily.$1.sstable",
        customer,
        i / kMicrosPerDay);

    if (!vfs_->exists(tbl)) {
      continue;
    }

    sstable::SSTableReader reader(vfs_->openFile(tbl));
    if (reader.bodySize() == 0 || reader.isFinalized() == 0) {
      continue;
    }

    sstable::SSTableScan scan;
    scan.setKeyPrefix(scan_common_prefix);

    auto cursor = reader.getCursor();
    scan.execute(cursor.get(), [&] (const Vector<String> row) {
      if (row.size() != 2) {
        RAISEF(kRuntimeError, "invalid row length: $0", row.size());
      }

      auto dims = StringUtil::split(row[0], "~");
      if (dims.size() != 4) {
        RAISEF(kRuntimeError, "invalid row key: $0", row[0]);
      }

      if (dims[0] != lang_str) {
        return;
      }

      if (test_groups.count(dims[1]) == 0) {
        return;
      }

      if (device_types.count(dims[2]) == 0) {
        return;
      }

      if (pages.size() > 0 && pages.count(dims[3]) == 0) {
        return;
      }

      auto counter = CTRCounterData::load(row[1]);
      ctr_ref.merge(counter);
      aggr_counter.merge(counter);
    });
  }

  /* write response */
  res->setStatus(http::kStatusOK);
  res->addHeader("Content-Type", "application/json; charset=utf-8");
  json::JSONOutputStream json(res->getBodyOutputStream());

  json.beginObject();
  json.addObjectEntry("aggregate");

  json.beginObject();
  json.addObjectEntry("views");
  json.addInteger(aggr_counter.num_views);
  json.addComma();
  json.addObjectEntry("clicks");
  json.addInteger(aggr_counter.num_clicks);
  json.addComma();
  json.addObjectEntry("ctr");
  json.addFloat(aggr_counter.num_clicks / (double) aggr_counter.num_views);
  json.addComma();
  json.addObjectEntry("cpq");
  json.addFloat(aggr_counter.num_clicked / (double) aggr_counter.num_views);
  json.endObject();

  json.addComma();
  json.addObjectEntry("timeseries");
  json.beginArray();
  int n = 0;
  for (const auto& c : counters) {
    if (++n > 1) {
      json.addComma();
    }

    json.beginObject();
    json.addObjectEntry("time");
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
    json.addObjectEntry("cpq");
    json.addFloat(c.second.num_clicked / (double) c.second.num_views);
    json.endObject();
  }

  json.endArray();
  json.endObject();
}

}
