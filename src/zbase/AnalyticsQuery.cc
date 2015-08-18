/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "stx/wallclock.h"
#include "zbase/AnalyticsQuery.h"

using namespace stx;

namespace cm {

AnalyticsQueryStatus::AnalyticsQueryStatus() :
    total_chunks(0),
    completed_chunks(0),
    rows_scanned(0) {}

double AnalyticsQueryStatus::progress() const {
  return completed_chunks / (double) total_chunks;
}

String AnalyticsQueryStatus::message() const {
  auto secs =
      (WallClock::unixMicros() - start_time.unixMicros()) /
      (double) kMicrosPerSecond;

  return StringUtil::format(
      "Running... $0% - $1 rows scanned ($2 rows/sec)",
      (completed_chunks / (double) total_chunks) * 100,
      rows_scanned,
      rows_scanned / secs);
}

void AnalyticsQueryStatus::onUpdate(Function<void ()> fn) {
  on_update = fn;
}

void AnalyticsQueryStatus::update(size_t chunks_delta, size_t rows_delta) {
  completed_chunks += chunks_delta;
  rows_scanned += rows_delta;
  on_update();
}

void AnalyticsQuery::build(const AnalyticsQuerySpec& spec) {
  customer = spec.customer();
  start_time = spec.start_time();
  end_time = spec.end_time();

  for (const auto& subq : spec.subqueries()) {
    queries.emplace_back(SubQueryParams { .query_type = subq.query() });
    for (const auto& p : subq.params()) {
      queries.back().params.emplace_back(p.key(), p.value());
    }
  }

  if (spec.segments().size() > 0) {
    loadSegmentsJSON(json::parseJSON(spec.segments()));
  }

  if (segments.size() == 0) {
    segments.emplace_back(TrafficSegmentParams {
      .key  = "all_traffic",
      .name = "All Traffic" });
  }

  static Vector<String> colors = {
      "#aa4643",
      "#4572a7",
      "#89a54e",
      "#80699b",
      "#3d96ae",
      "#db843d"
  };

  for (int n = 0; n < segments.size(); ++n) {
    segments[n].color = colors[n % colors.size()];
  }
}

void AnalyticsQuery::rewrite() {
  start_time = (start_time / kMicrosPerDay) * kMicrosPerDay;
  end_time = ((end_time / kMicrosPerDay) + 1) * kMicrosPerDay;

  for (auto& s : segments) {
    for (auto& r : s.rules) {

      /* queries.language */
      if (std::get<0>(r) == "search_queries.language") {
        std::get<1>(r) = TrafficSegmentOp::MATCHES_UINT32;

        for (int i = 0; i < std::get<2>(r).size(); ++i) {
          std::get<2>(r)[i] = StringUtil::toString(
              (uint32_t) languageFromString(std::get<2>(r)[i]));
        }

        continue;
      }

      /* queries.device_type */
      if (std::get<0>(r) == "search_queries.device_type") {
        std::get<1>(r) = TrafficSegmentOp::MATCHES_UINT32;

        for (int i = 0; i < std::get<2>(r).size(); ++i) {
          std::get<2>(r)[i] = StringUtil::toString(
              (uint32_t) deviceTypeFromString(std::get<2>(r)[i]));
        }

        continue;
      }

      /* queries.page_type */
      if (std::get<0>(r) == "search_queries.page_type") {
        std::get<1>(r) = TrafficSegmentOp::MATCHES_UINT32;

        for (int i = 0; i < std::get<2>(r).size(); ++i) {
          std::get<2>(r)[i] = StringUtil::toString(
              (uint32_t) pageTypeFromString(std::get<2>(r)[i]));
        }

        continue;
      }

      /* queries.ab_test_group */
      if (std::get<0>(r) == "search_queries.ab_test_group" ||
          std::get<0>(r) == "ab_test_group" ) {
        std::get<1>(r) = TrafficSegmentOp::MATCHES_UINT32;
        continue;
      }

    }
  }
}

void AnalyticsQuery::toJSON(json::JSONOutputStream* json) const {
  json->beginObject();
  json->addObjectEntry("start_time");
  json->addFloat(start_time);
  json->addComma();
  json->addObjectEntry("end_time");
  json->addFloat(end_time);
  json->addComma();

  json->addObjectEntry("segments");
  json->beginArray();
  for (int i = 0; i < segments.size(); ++i) {
    if (i > 0) json->addComma();
    segments[i].toJSON(json);
  }
  json->endArray();
  json->endObject();
}

void AnalyticsQuery::loadSegmentsJSON(const json::JSONObject& json) {
  auto n = json::JSONUtil::arrayLength(json.begin(), json.end());

  for (int i = 0; i < n; ++i) {
    TrafficSegmentParams params;

    auto seg = json::JSONUtil::arrayLookup(json.begin(), json.end(), i);
    auto key = json::JSONUtil::objectGetString(seg, json.end(), "key");
    auto name = json::JSONUtil::objectGetString(seg, json.end(), "name");
    auto color = json::JSONUtil::objectGetString(seg, json.end(), "color");
    auto rules = json::JSONUtil::objectLookup(seg, json.end(), "rules");

    if (key.isEmpty()) {
      RAISE(kIllegalArgumentError, "missing 'key' parameter");
    }

    if (name.isEmpty()) {
      RAISE(kIllegalArgumentError, "missing 'name' parameter");
    }

    params.key = key.get();
    params.name = name.get();
    params.color = color.isEmpty() ? "#000" : color.get();

    auto r = json::JSONUtil::arrayLength(rules, json.end());
    for (int j = 0; j < r; ++j) {
      auto rule = json::JSONUtil::arrayLookup(rules, json.end(), j);
      auto field = json::JSONUtil::objectGetString(rule, json.end(), "field");
      auto op = json::JSONUtil::objectGetString(rule, json.end(), "op");
      auto values = json::JSONUtil::objectLookup(rule, json.end(), "values");

      if (field.isEmpty()) {
        RAISE(kIllegalArgumentError, "missing 'field' parameter");
      }

      if (values == json.end()) {
        RAISE(kIllegalArgumentError, "missing 'values' parameter");
      }

      Vector<String> vals;
      auto v = json::JSONUtil::arrayLength(values, json.end());
      for (int l = 0; l < v; ++l) {
        auto val = json::JSONUtil::arrayGetString(values, json.end(), l);
        if (val.isEmpty()) {
          RAISE(kIllegalArgumentError, "invalid 'values' parameter");
        }

        vals.emplace_back(val.get());
      }

      params.rules.emplace_back(
          field.get(),
          trafficSegmentOpFromString(op.get()),
          vals);
    }

    segments.emplace_back(params);
  }
}

} // namespace cm

