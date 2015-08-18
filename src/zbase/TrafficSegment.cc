/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <stx/Language.h>
#include "zbase/TrafficSegment.h"

using namespace stx;

namespace cm {

TrafficSegment::TrafficSegment(
    const TrafficSegmentParams& params,
    AnalyticsTableScan* scan) :
    params_(params) {
  for (const auto& r : params.rules) {
    addRule(r, scan);
  }
}

bool TrafficSegment::checkPredicates() {
  for (const auto& pred : predicates_) {
    if (!pred()) {
      return false;
    }
  }

  return true;
}

const String& TrafficSegment::key() const {
  return params_.key;
}

void TrafficSegment::addRule(
    const TrafficSegmentRule& rule,
    AnalyticsTableScan* scan) {
  switch (std::get<1>(rule)) {

    /* MATCHES */
    case TrafficSegmentOp::MATCHES: {
      auto col = scan->fetchColumn(std::get<0>(rule));
      Set<String> vals;

      for (const auto& v: std::get<2>(rule)) {
        vals.emplace(v);
      }

      if (vals.size() == 1) {
        auto val = *vals.begin();
        predicates_.emplace_back([col, val] () {
          return col->getString() == val;
        });
      } else {
        predicates_.emplace_back([col, vals] () {
          return vals.count(col->getString()) > 0;
        });
      }

      return;
    }

    /* MATCHES_UINT32 */
    case TrafficSegmentOp::MATCHES_UINT32: {
      auto col = scan->fetchColumn(std::get<0>(rule));
      Set<uint32_t> vals;

      for (const auto& v: std::get<2>(rule)) {
        vals.emplace((uint32_t) std::stoul(v));
      }

      if (vals.size() == 1) {
        auto val = *vals.begin();
        predicates_.emplace_back([col, val] () {
          return col->getUInt32() == val;
        });
      } else {
        predicates_.emplace_back([col, vals] () {
          return vals.count(col->getUInt32()) > 0;
        });
      }

      return;
    }

    /* INCLUDESI */
    case TrafficSegmentOp::INCLUDESI: {
      auto col = scan->fetchColumn(std::get<0>(rule));
      auto vals = std::get<2>(rule);

      predicates_.emplace_back([col, vals] () -> bool {
        auto s = col->getString();

        for (const auto& v : vals) {
          if (s.find(v) != String::npos) {
            return true;
          }
        }

        return false;
      });

      return;
    }

  }
}

void TrafficSegmentParams::toJSON(json::JSONOutputStream* json) const {
  json->beginObject();
  json->addObjectEntry("key");
  json->addString(key);
  json->addComma();
  json->addObjectEntry("name");
  json->addString(name);
  json->addComma();
  json->addObjectEntry("color");
  json->addString(color);
  json->addComma();
  json->addObjectEntry("rules");

  json->beginArray();
  for (int i = 0; i < rules.size(); ++i) {
    if (i > 0) {
      json->addComma();
    }

    json->beginObject();
    json->addObjectEntry("field");
    json->addString(std::get<0>(rules[i]));
    json->addComma();
    json->addObjectEntry("op");
    json->addString(trafficSegmentOpToString(std::get<1>(rules[i])));
    json->addComma();
    json->addObjectEntry("values");
    json->beginArray();

    int j = 0;
    for (const auto& v : std::get<2>(rules[i])) {
      if (++j > 1) json->addComma();
      json->addString(v);
    }

    json->endArray();
    json->endObject();
  }
  json->endArray();

  json->endObject();
}

TrafficSegmentOp trafficSegmentOpFromString(String op) {
  StringUtil::toLower(&op);

  if (op == "matches") return TrafficSegmentOp::MATCHES;
  if (op == "matches_uint32") return TrafficSegmentOp::MATCHES_UINT32;
  if (op == "includesi") return TrafficSegmentOp::INCLUDESI;

  RAISEF(kIndexError, "invalid segment operation: $0", op);
}

String trafficSegmentOpToString(TrafficSegmentOp op) {
  switch (op) {
    case TrafficSegmentOp::MATCHES: return "MATCHES";
    case TrafficSegmentOp::MATCHES_UINT32: return "MATCHES_UINT32";
    case TrafficSegmentOp::INCLUDESI: return "INCLUDESI";
  }
}

} // namespace cm
