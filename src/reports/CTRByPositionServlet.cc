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
#include "fnord-base/io/fileutil.h"
#include "fnord-sstable/sstablereader.h"
#include "fnord-sstable/SSTableScan.h"

using namespace fnord;

namespace cm {

CTRByPositionServlet::CTRByPositionServlet(VFS* vfs) : vfs_(vfs) {}

void CTRByPositionServlet::handleHTTPRequest(
    http::HTTPRequest* req,
    http::HTTPResponse* res) {
  URI uri(req->uri());

  const auto& params = uri.queryParams();

  /* arguments */
  String customer;
  if (!URI::getParam(params, "customer", &customer)) {
    res->addBody("error: missing ?customer=... parameter");
    res->setStatus(http::kStatusBadRequest);
    return;
  }

  Set<String> test_groups { "all" };
  Set<String> device_types { "unknown", "desktop", "tablet", "phone" };

  Language lang = Language::DE;
  auto lang_str = languageToString(lang);

  /* prepare prefix filters for scanning */
  String scan_common_prefix = languageToString(lang) + "~";
  if (test_groups.size() == 1) {
    scan_common_prefix += *test_groups.begin() + "~";
  }

  if (device_types.size() == 1) {
    scan_common_prefix += *device_types.begin() + "~";
  }

  /* scan input tables */
  Set<String> tables { "dawanda_ctr_by_position.99066.sstable" };

  HashMap<uint64_t, CTRCounterData> counters;
  for (const auto& tbl : tables) {
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

      auto counter = CTRCounterData::load(row[1]);
      auto posi = std::stoul(dims[3]);

      counters[posi].merge(counter);
    });
  }


  /* write response */
  res->setStatus(http::kStatusOK);
  res->addHeader("Content-Type", "application/json; charset=utf-8");
  json::JSONOutputStream json(res->getBodyOutputStream());

  json.beginArray();

  uint64_t total_views = 0;
  uint64_t total_clicks = 0;

  for (const auto& c : counters) {
    total_views += c.second.num_views;
    total_clicks += c.second.num_clicks;
  }

  int n = 0;
  for (const auto& c : counters) {
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
}

}
