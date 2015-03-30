/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "reports/CTRCounterMergeReducer.h"

using namespace fnord;

namespace cm {

CTRCounterMergeReducer::CTRCounterMergeReducer(
    RefPtr<CTRCounterTableSource> input,
    RefPtr<CTRCounterTableSink> output) :
    Report(input.get(), output.get()),
    input_table_(input),
    output_table_(output) {}

void CTRCounterMergeReducer::onInit() {
  input_table_->forEach(std::bind(
      &CTRCounterMergeReducer::onCTRCounter,
      this,
      std::placeholders::_1,
      std::placeholders::_2));
}

void CTRCounterMergeReducer::onCTRCounter(const String& key, const CTRCounterData& c) {
  counters_[key].merge(c);
}

void CTRCounterMergeReducer::onFinish() {
  for (auto& ctr : counters_) {
    output_table_->addRow(ctr.first, ctr.second);
  }
}

} // namespace cm

