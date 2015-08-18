/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "zbase/catalog/CatalogCategoryDashboardResult.h"

using namespace stx;

namespace zbase {

void CatalogCategoryDashboardResult::merge(
    const AnalyticsQueryResult::SubQueryResult& o) {
  const auto& other = dynamic_cast<const CatalogCategoryDashboardResult&>(o);
  parent.merge(other.parent);
  children.merge(other.children);
}

void CatalogCategoryDashboardResult::toJSON(json::JSONOutputStream* json) const {
  json->beginArray();

  for (int n = 0; n < parent.segment_keys.size(); ++n) {
    SearchCTRStats parent_aggr;
    for (const auto& p : parent.series[n]) {
      parent_aggr.merge(p.second);
    }

    if (n > 0) {
      json->addComma();
    }

    json->beginObject();
    json->addObjectEntry("segment");
    json->addString(parent.segment_keys[n]);
    json->addComma();

    json->addObjectEntry("parent_aggregate");
    parent_aggr.toJSON(json);
    json->addComma();

    json->addObjectEntry("parent_timeseries");
    json->beginArray();
    int i = 0;
    for (const auto& p : parent.series[n]) {
      if (++i > 1) {
        json->addComma();
      }

      json->beginObject();
      json->addObjectEntry("time");
      json->addInteger(p.first);
      json->addComma();
      json->addObjectEntry("data");
      p.second.toJSON(json);
      json->endObject();
    }
    json->endArray();
    json->addComma();

    json->addObjectEntry("children");
    json->beginArray();

    i = 0;
    for (const auto& c : children.counters[n]) {
      if (++i > 1) {
        json->addComma();
      }

      json->beginObject();
      json->addObjectEntry("category");
      json->addString(StringUtil::toString(c.first));
      json->addComma();
      json->addObjectEntry("data");
      c.second.toJSON(json);
      json->endObject();
    }

    json->endArray();
    json->endObject();
  }

  json->endArray();
}

void CatalogCategoryDashboardResult::encode(
    util::BinaryMessageWriter* writer) const {
  parent.encode(writer);
  children.encode(writer);
}

void CatalogCategoryDashboardResult::decode(util::BinaryMessageReader* reader) {
  parent.decode(reader);
  children.decode(reader);
}

void CatalogCategoryDashboardResult::applyTimeRange(
    UnixTime from,
    UnixTime until) {
  parent.applyTimeRange(from, until);
}

} // namespace zbase

