/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <google/protobuf/text_format.h>
#include "zbase/discovery/ItemBoostExport.h"

using namespace stx;

namespace zbase {

ItemBoostExport::ItemBoostExport(
    RefPtr<ProtoSSTableSource<ItemBoostRow>> input,
    RefPtr<CSVSink> output) :
    ReportRDD(input.get(), output.get()),
    input_(input),
    output_(output) {}

void ItemBoostExport::onInit() {
  input_->forEach(std::bind(
      &ItemBoostExport::onRow,
      this,
      std::placeholders::_1,
      std::placeholders::_2));

  Vector<String> headers;
  headers.emplace_back("product_id");
  headers.emplace_back("popularity_score");
  headers.emplace_back("conversion_score");
  headers.emplace_back("top_terms");
}

void ItemBoostExport::onRow(const String& key, const ItemBoostRow& r) {
  auto row = r;
  double total_clicks = row.num_clicks();

  Vector<ItemBoostTermInfo> top_terms;
  for (const auto& term : row.top_terms()) {
    if (term.score() < 10 || term.score() / total_clicks < 0.05f) {
      continue;
    }

    top_terms.emplace_back(term);
  }

  row.mutable_top_terms()->Clear();

  for (const auto& term : top_terms) {
    *row.add_top_terms() = term;
  }

  rows_.emplace_back(key, row);
}

void ItemBoostExport::onFinish() {
  auto min_imprs = 1000;
  auto min_clicks = 100;

  /* calculate ctr mean, mean nclicks */
  double ctr_mean_num = 0.0;
  double ctr_mean_den = 0;
  double clicks_mean_num = 0.0;
  double clicks_mean_den = 0.0;
  for (const auto& s : rows_) {
    auto nimprs = s.second.num_impressions();
    auto nclicks = s.second.num_clicks();
    auto ctr = nclicks / (double) nimprs;
    if (nimprs < min_imprs || nclicks < min_clicks) {
      continue;
    }

    clicks_mean_num += nclicks;
    clicks_mean_den += 1;

    ctr_mean_num += ctr;
    ctr_mean_den += 1;
  }

  auto ctr_mean = ctr_mean_num / ctr_mean_den;
  auto clicks_mean = clicks_mean_num / clicks_mean_den;

  /* calculate ctr stddev, nclicks stddev */
  double ctr_stddev_num = 0.0;
  double ctr_stddev_den = 0.0;
  double clicks_stddev_num = 0.0;
  double clicks_stddev_den = 0.0;
  for (const auto& s : rows_) {
    auto nimprs = s.second.num_impressions();
    auto nclicks = s.second.num_clicks();
    auto ctr = nclicks / (double) nimprs;
    if (nimprs < min_imprs || nclicks < min_clicks) {
      continue;
    }

    clicks_stddev_num += pow(nclicks - clicks_mean, 2.0);
    clicks_stddev_den += 1;

    ctr_stddev_num += pow(ctr - ctr_mean, 2.0);
    ctr_stddev_den += 1;
  }

  auto ctr_stddev = sqrt(ctr_stddev_num / ctr_stddev_den);
  auto clicks_stddev = sqrt(clicks_stddev_num / clicks_stddev_den);

  stx::iputs("clicks mean=$0 stddev=$1", clicks_mean, clicks_stddev);
  stx::iputs("ctr mean=$0 stddev=$1", ctr_mean, ctr_stddev);

  for (auto& s : rows_) {
    auto nimprs = s.second.num_impressions();
    auto nclicks = s.second.num_clicks();
    auto ctr = nclicks / (double) nimprs;

    auto conversion_score = (ctr - ctr_mean) / ctr_stddev;
    auto conversion_score_clip = 3.0f;
    if (conversion_score > conversion_score_clip) {
      conversion_score = conversion_score_clip;
    } else if (conversion_score < conversion_score_clip * -1) {
      conversion_score = conversion_score_clip * -1;
    }
    conversion_score /= conversion_score_clip;

    auto popularity_score = (nclicks - clicks_mean) / clicks_stddev;
    if (popularity_score < 0) {
      popularity_score = 0;
    }

    if (nimprs < min_imprs || nclicks < min_clicks) {
      popularity_score = 0;
      conversion_score = 0;
    }

    String top_terms;
    for (const auto& term : s.second.top_terms()) {
      top_terms += term.term() + ",";
    }

    Vector<String> cols;
    cols.emplace_back(s.first.substr(2));
    cols.emplace_back(StringUtil::toString(popularity_score));
    cols.emplace_back(StringUtil::toString(conversion_score));
    cols.emplace_back(top_terms);

    output_->addRow(cols);
  }
}

} // namespace zbase

