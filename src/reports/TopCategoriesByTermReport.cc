/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "reports/TopCategoriesByTermReport.h"

using namespace fnord;

namespace cm {

TopCategoriesByTermReport::TopCategoriesByTermReport(
    RefPtr<CTRCounterTableSource> input,
    RefPtr<TermInfoTableSink> output,
    const String& cat_prefix) :
    Report(input.get(), output.get()),
    input_table_(input),
    output_table_(output),
    cat_prefix_(cat_prefix) {}

void TopCategoriesByTermReport::onInit() {
  input_table_->forEach(std::bind(
      &TopCategoriesByTermReport::onCTRCounter,
      this,
      std::placeholders::_1,
      std::placeholders::_2));
}

void TopCategoriesByTermReport::onCTRCounter(
    const String& key,
    const CTRCounterData& c) {
  auto t_end = StringUtil::findLast(key, '~');
  if (t_end == String::npos) {
    return;
  }

  auto term = key.substr(0, t_end);
  auto cat_id = key.substr(t_end + 1);

  fnord::iputs("term: $0, cat: $1", term, cat_id);
}

void TopCategoriesByTermReport::onFinish() {
  for (const auto& c : counters_) {
    output_table_->addRow(c.first , c.second);
  }
}

} // namespace cm

