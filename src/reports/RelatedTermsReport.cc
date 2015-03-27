/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "reports/RelatedTermsReport.h"

using namespace fnord;

namespace cm {

RelatedTermsReport::RelatedTermsReport(
    RefPtr<CTRCounterTableSource> input,
    RefPtr<TermInfoTableSink> output) :
    Report(input.get(), output.get()),
    input_table_(input),
    output_table_(output) {}

void RelatedTermsReport::onInit() {
  input_table_->forEach(std::bind(
      &RelatedTermsReport::onCTRCounter,
      this,
      std::placeholders::_1,
      std::placeholders::_2));
}

void RelatedTermsReport::onCTRCounter(
    const String& key,
    const CTRCounterData& c) {
  auto t_begin = StringUtil::find(key, '~');
  if (t_begin == String::npos) {
    return;
  }

  auto lang = key.substr(0, t_begin);
  auto& l_map = counters_[lang];

  // query was already normalized
  auto terms = StringUtil::split(key.substr(t_begin + 1), " ");

  for (int j = 0; j < terms.size(); ++j) {
    auto& t_map = l_map[terms[j]];

    for (int i = 0; i < terms.size(); ++i) {
      if (i == j) {
        continue;
      }

      t_map[terms[i]] += c.num_clicks;
    }
  }
}

void RelatedTermsReport::onFinish() {
  for (const auto& l : counters_) {
    for (const auto& t : l.second) {
      output_table_->addRow(l.first, t.first, t.second);
    }
  }
}

} // namespace cm

