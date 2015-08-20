/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <algorithm>
#include "TopSearchTermsQuery.h"
#include "stx/uri.h"
#include "stx/util/CumulativeHistogram.h"
#include "cstable/BitPackedIntColumnReader.h"

using namespace stx;

namespace zbase {

TopSearchTermsQuery::TopSearchTermsQuery(
    AnalyticsTableScan* query,
    const Vector<RefPtr<TrafficSegment>>& segments,
    const AnalyticsQuery::SubQueryParams& params) :
    result_(new TopSearchQueriesResult()),
    segments_(segments),
    time_col_(query->fetchColumn("search_queries.time")),
    page_col_(query->fetchColumn("search_queries.page")),
    qstr_col_(query->fetchColumn("search_queries.query_string")),
    clicks_col_(query->fetchColumn("search_queries.num_result_items_clicked")),
    gmv_col_(query->fetchColumn("search_queries.gmv_eurcents")),
    cart_value_col_(query->fetchColumn("search_queries.cart_value_eurcents")) {
  query->onQuery(std::bind(&TopSearchTermsQuery::onQuery, this));

  for (const auto& s : segments_) {
    result_->segment_keys.emplace_back(s->key());
    result_->counters.emplace_back();
  }

  result_->limit = 100;
  String limit_str;
  if (URI::getParam(params.params, "limit", &limit_str)) {
    result_->limit = std::stoull(limit_str);
  }

  result_->offset = 0;
  String offset_str;
  if (URI::getParam(params.params, "offset", &offset_str)) {
    result_->offset = std::stoull(offset_str);
  }

  result_->order_by = "views";
  URI::getParam(params.params, "order_by", &result_->order_by);

  result_->order_fn = "numdsc";
  URI::getParam(params.params, "order_fn", &result_->order_fn);
}

void TopSearchTermsQuery::onQuery() {
  auto page = page_col_->getUInt32();
  if (page != 1) {
    return;
  }

  auto qstr = qstr_col_->getString();
  if (qstr.length() < 1) {
    return;
  }

  auto clicks = clicks_col_->getUInt32();
  auto gmv = gmv_col_->getUInt32();
  auto cart_value = cart_value_col_->getUInt32();
  for (int i = 0; i < segments_.size(); ++i) {
    if (!segments_[i]->checkPredicates()) {
      continue;
    }

    auto& counter = result_->counters[i][qstr];
    ++counter.num_views;
    counter.num_clicks += clicks > 0 ? 1 : 0;
    counter.num_clicked += clicks;
    counter.gmv_eurcent += gmv;
    counter.cart_value_eurcent += cart_value;
  }
}

RefPtr<AnalyticsQueryResult::SubQueryResult> TopSearchTermsQuery::result() {
  return result_.get();
}

static double mkRate(double num, double div) {
  if (div == 0 || num == 0) {
    return 0;
  }

  return num / div;
}

void TopSearchQueriesResult::toJSON(json::JSONOutputStream* json) const {
  json->beginArray();

  auto segs = GroupResult<String, CTRCounterData>::segment_keys.size();
  for (int n = 0; n < segs; ++n) {
    uint64_t total_views = 0;
    uint64_t total_clicks = 0;

    Vector<CTRCounter> counters;
    auto bs = GroupResult<String, CTRCounterData>::counters[n].size() / 200.0f;
    auto views_histo_cumul = util::CumulativeHistogram::withLinearBins(bs);
    auto clicks_histo_cumul = util::CumulativeHistogram::withLinearBins(bs);

    for (const auto& c : GroupResult<String, CTRCounterData>::counters[n]) {
      total_views += c.second.num_views;
      total_clicks += c.second.num_clicks;
      counters.emplace_back(c);
    }

    sortResults(&counters);

    for (int i = 0; i < counters.size(); ++i) {
      const auto& c = counters[i];
      views_histo_cumul.addDatum(i, c.second.num_views);
      clicks_histo_cumul.addDatum(i, c.second.num_clicks);
    }

    if (n > 0) {
      json->addComma();
    }

    json->beginObject();
    json->addObjectEntry("segment");
    json->addString(GroupResult<String, CTRCounterData>::segment_keys[n]);
    json->addComma();
    json->addObjectEntry("views_histo_cumul");
    json::toJSON(views_histo_cumul.cumulativeRelativeHistogram(), json);
    json->addComma();
    json->addObjectEntry("clicks_histo_cumul");
    json::toJSON(clicks_histo_cumul.cumulativeRelativeHistogram(), json);
    json->addComma();
    json->addObjectEntry("toplist");
    json->beginArray();

    double viewshare_cumul = 0;
    double clickshare_cumul = 0;
    size_t x = 0;
    for (int i = offset; i < counters.size() && i < offset + limit; ++i) {
      const auto& c = counters[i];
      auto viewshare = c.second.num_views / (double) total_views;
      auto clickshare = c.second.num_clicks / (double) total_clicks;
      viewshare_cumul += viewshare;
      clickshare_cumul += clickshare;

      if (++x > 1) {
        json->addComma();
      }

      json->beginObject();
      json->addObjectEntry("key");
      json->addString(StringUtil::toString(c.first));
      json->addComma();
      json->addObjectEntry("views");
      json->addInteger(c.second.num_views);
      json->addComma();
      json->addObjectEntry("clicks");
      json->addInteger(c.second.num_clicks);
      json->addComma();
      json->addObjectEntry("ctr");
      json->addFloat(mkRate(c.second.num_clicks, c.second.num_views));
      json->addComma();
      json->addObjectEntry("cpq");
      json->addFloat(mkRate(c.second.num_clicked, c.second.num_views));
      json->addComma();
      json->addObjectEntry("total_gmv_eurcent");
      json->addInteger(c.second.gmv_eurcent);
      json->addComma();
      json->addObjectEntry("total_cart_value_eurcent");
      json->addInteger(c.second.cart_value_eurcent);
      json->addComma();
      json->addObjectEntry("avg_gmv_eurcent");
      json->addInteger(mkRate(c.second.gmv_eurcent, c.second.num_views));
      json->addComma();
      json->addObjectEntry("avg_cart_value_eurcent");
      json->addInteger(mkRate(c.second.cart_value_eurcent, c.second.num_views));
      json->addComma();
      json->addObjectEntry("clickshare");
      json->addFloat(clickshare);
      json->addComma();
      json->addObjectEntry("clickshare_cumul");
      json->addFloat(clickshare_cumul);
      json->addComma();
      json->addObjectEntry("viewshare");
      json->addFloat(viewshare);
      json->addComma();
      json->addObjectEntry("viewshare_cumul");
      json->addFloat(viewshare_cumul);
      json->endObject();
    }

    json->endArray();
    json->endObject();
  }

  json->endArray();
}

void TopSearchQueriesResult::sortResults(Vector<CTRCounter>* counters) const {
  /* views */
  if (order_by == "views") {
    if (order_fn == "numasc") {
      std::sort(counters->begin(), counters->end(), [] (
          const CTRCounter& a,
          const CTRCounter& b) -> bool {
        return a.second.num_views < b.second.num_views;
      });
    } else {
      std::sort(counters->begin(), counters->end(), [] (
          const CTRCounter& a,
          const CTRCounter& b) -> bool {
        return b.second.num_views < a.second.num_views;
      });
    }

    return;
  }

  /* clicks */
  if (order_by == "clicks") {
    if (order_fn == "numasc") {
      std::sort(counters->begin(), counters->end(), [] (
          const CTRCounter& a,
          const CTRCounter& b) -> bool {
        return a.second.num_clicks < b.second.num_clicks;
      });
    } else {
      std::sort(counters->begin(), counters->end(), [] (
          const CTRCounter& a,
          const CTRCounter& b) -> bool {
        return b.second.num_clicks < a.second.num_clicks;
      });
    }

    return;
  }

  /* gmv_eurcent */
  if (order_by == "total_gmv_eurcent") {
    if (order_fn == "numasc") {
      std::sort(counters->begin(), counters->end(), [] (
          const CTRCounter& a,
          const CTRCounter& b) -> bool {
        return a.second.gmv_eurcent < b.second.gmv_eurcent;
      });
    } else {
      std::sort(counters->begin(), counters->end(), [] (
          const CTRCounter& a,
          const CTRCounter& b) -> bool {
        return b.second.gmv_eurcent < a.second.gmv_eurcent;
      });
    }

    return;
  }

  /* cart_value_eurcent */
  if (order_by == "total_cart_value_eurcent") {
    if (order_fn == "numasc") {
      std::sort(counters->begin(), counters->end(), [] (
          const CTRCounter& a,
          const CTRCounter& b) -> bool {
        return a.second.cart_value_eurcent < b.second.cart_value_eurcent;
      });
    } else {
      std::sort(counters->begin(), counters->end(), [] (
          const CTRCounter& a,
          const CTRCounter& b) -> bool {
        return b.second.cart_value_eurcent < a.second.cart_value_eurcent;
      });
    }

    return;
  }

  /* ctr */
  if (order_by == "ctr") {
    if (order_fn == "numasc") {
      std::sort(counters->begin(), counters->end(), [] (
          const CTRCounter& a,
          const CTRCounter& b) -> bool {
        return mkRate(a.second.num_clicks, a.second.num_views) <
            mkRate(b.second.num_clicks, b.second.num_views);
      });
    } else {
      std::sort(counters->begin(), counters->end(), [] (
          const CTRCounter& a,
          const CTRCounter& b) -> bool {
        return mkRate(b.second.num_clicks, b.second.num_views) <
            mkRate(a.second.num_clicks, a.second.num_views);
      });
    }

    return;
  }

  /* cpq */
  if (order_by == "cpq") {
    if (order_fn == "numasc") {
      std::sort(counters->begin(), counters->end(), [] (
          const CTRCounter& a,
          const CTRCounter& b) -> bool {
        return mkRate(a.second.num_clicked, a.second.num_views) <
            mkRate(b.second.num_clicked, b.second.num_views);
      });
    } else {
      std::sort(counters->begin(), counters->end(), [] (
          const CTRCounter& a,
          const CTRCounter& b) -> bool {
        return mkRate(b.second.num_clicked, b.second.num_views) <
            mkRate(a.second.num_clicked, a.second.num_views);
      });
    }

    return;
  }

  RAISEF(kRuntimeError, "invalid order_by parameter: '$0'", order_by);
}

void TopSearchQueriesResult::toCSV(CSVOutputStream* csv) const {
  Vector<String> headers;

  auto has_segment_col = columns.empty() || columns.count("segment") > 0;
  if (has_segment_col) {
    headers.emplace_back("segment");
  }

  auto has_key_col = columns.empty() || columns.count("key") > 0;
  if (has_key_col) {
    headers.emplace_back("key");
  }

  auto has_clicks_col = columns.empty() || columns.count("num_clicks") > 0;
  if (has_clicks_col) {
    headers.emplace_back("clicks");
  }

  auto has_views_col = columns.empty() || columns.count("num_views") > 0;
  if (has_views_col) {
    headers.emplace_back("views");
  }

  auto has_ctr_col = columns.empty() || columns.count("ctr") > 0;
  if (has_ctr_col) {
    headers.emplace_back("ctr");
  }

  auto has_cpq_col = columns.empty() || columns.count("cpq") > 0;
  if (has_cpq_col) {
    headers.emplace_back("cpq");
  }

  auto has_total_cart_value_col =
    columns.empty() || columns.count("total_cart_value_eurcent") > 0;
  if (has_total_cart_value_col) {
    headers.emplace_back("total_cart_value_eurcent");
  }

  auto has_total_gmv_col =
    columns.empty() || columns.count("total_gmv_eurcent") > 0;
  if (has_total_gmv_col) {
    headers.emplace_back("total_gmv_eurcent");
  }

  auto has_avg_cart_value_col =
    columns.empty() || columns.count("avg_cart_value_eurcent") > 0;
  if (has_avg_cart_value_col) {
    headers.emplace_back("avg_cart_value_eurcent");
  }

  auto has_avg_gmv_col =
    columns.empty() || columns.count("avg_gmv_eurcent") > 0;
  if (has_avg_gmv_col) {
    headers.emplace_back("avg_gmv_eurcent");
  }

  csv->appendRow(headers);

  auto segs = GroupResult<String, CTRCounterData>::segment_keys.size();
  for (int n = 0; n < segs; ++n) {
    Vector<CTRCounter> counters;
    for (const auto& c : GroupResult<String, CTRCounterData>::counters[n]) {
      counters.emplace_back(c);
    }

    sortResults(&counters);
    for (const auto& c : counters) {
      Vector<String> row;

      if (has_segment_col) {
        row.emplace_back(GroupResult<String, CTRCounterData>::segment_keys[n]);
      }

      if (has_key_col) {
        row.emplace_back(c.first);
      }

      if (has_clicks_col) {
        row.emplace_back(StringUtil::toString(c.second.num_clicks));
      }

      if (has_views_col) {
        row.emplace_back(StringUtil::toString(c.second.num_views));
      }

      if (has_ctr_col) {
        row.emplace_back(StringUtil::toString(
          mkRate(c.second.num_clicks, c.second.num_views)));
      }

      if (has_cpq_col) {
        row.emplace_back(StringUtil::toString(
          mkRate(c.second.num_clicked, c.second.num_views)));
      }

      if (has_total_cart_value_col) {
        row.emplace_back(StringUtil::toString(c.second.cart_value_eurcent));
      }

      if (has_total_gmv_col) {
        row.emplace_back(StringUtil::toString(c.second.gmv_eurcent));
      }

      if (has_avg_cart_value_col) {
        row.emplace_back(StringUtil::toString(
          mkRate(c.second.cart_value_eurcent, c.second.num_views)));
      }

      if (has_avg_gmv_col) {
        row.emplace_back(StringUtil::toString(
          mkRate(c.second.gmv_eurcent, c.second.num_views)));
      }

      csv->appendRow(row);
    }
  }
}

} // namespace zbase

