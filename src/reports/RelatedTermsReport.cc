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
    RefPtr<CTRCounterTableSink> output) :
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
  fnord::iputs("key: $0 -> views $1, clicks $2", key, c.num_views, c.num_clicks);
  counters_[key].merge(c);
}

void RelatedTermsReport::onFinish() {
  //for (auto& ctr : counters_) {
  //  output_table_->addRow(ctr.first, ctr.second);
  //}
}

} // namespace cm

