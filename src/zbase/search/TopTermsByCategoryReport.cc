/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <algorithm>
#include "zbase/search/TopTermsByCategoryReport.h"

using namespace stx;

namespace cm {

TopTermsByCategoryReport::TopTermsByCategoryReport(
    RefPtr<CTRCounterTableSource> input,
    RefPtr<CSVSink> output) :
    ReportRDD(input.get(), output.get()),
    input_table_(input),
    output_table_(output) {}

void TopTermsByCategoryReport::onInit() {
  input_table_->forEach(std::bind(
      &TopTermsByCategoryReport::onCTRCounter,
      this,
      std::placeholders::_1,
      std::placeholders::_2));
}

void TopTermsByCategoryReport::onCTRCounter(
    const String& key,
    const CTRCounterData& c) {
  auto t_end = StringUtil::findLast(key, '~');
  if (t_end == String::npos) {
    return;
  }

  if (t_end < 4) {
    return;
  }

  auto term = key.substr(3, t_end - 3);
  auto cat_id = key.substr(t_end + 1);

  if (term.size() < 2) {
    return;
  }

  counters_[cat_id].emplace_back(term, c.num_clicks);
}

void TopTermsByCategoryReport::onFinish() {
  output_table_->addRow(Vector<String> {
    "category_id",
    "term",
    "score"
  });

  for (auto& c : counters_) {
    std::sort(c.second.begin(), c.second.end(), [] (
        const Pair<String, uint64_t>& a,
        const Pair<String, uint64_t>& b) {
      return b.second < a.second;
    });

    for (int i = 0; i < c.second.size() && i < 25; ++i) {
      Vector<String> row;
      row.emplace_back(c.first);
      row.emplace_back(c.second[i].first);
      row.emplace_back(StringUtil::toString(c.second[i].second));
      output_table_->addRow(row);
    }
  }

  counters_.clear();
}

} // namespace cm

