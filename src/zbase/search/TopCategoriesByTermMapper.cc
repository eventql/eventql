/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "zbase/search/TopCategoriesByTermMapper.h"

using namespace stx;

namespace cm {

TopCategoriesByTermMapper::TopCategoriesByTermMapper(
    RefPtr<CTRCounterTableSource> input,
    RefPtr<TermInfoTableSink> output,
    const String& cat_prefix) :
    ReportRDD(input.get(), output.get()),
    input_table_(input),
    output_table_(output),
    cat_prefix_(cat_prefix) {}

void TopCategoriesByTermMapper::onInit() {
  input_table_->forEach(std::bind(
      &TopCategoriesByTermMapper::onCTRCounter,
      this,
      std::placeholders::_1,
      std::placeholders::_2));
}

void TopCategoriesByTermMapper::onCTRCounter(
    const String& key,
    const CTRCounterData& c) {
  auto t_end = StringUtil::findLast(key, '~');
  if (t_end == String::npos) {
    return;
  }

  auto term = key.substr(0, t_end);
  auto cat_id = cat_prefix_ + key.substr(t_end + 1);

  counters_[term].top_categories[cat_id] += c.num_clicks;
}

void TopCategoriesByTermMapper::onFinish() {
  for (const auto& c : counters_) {
    output_table_->addRow(c.first , c.second);
  }
}

} // namespace cm

