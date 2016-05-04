/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "eventql/TermInfoMergeReducer.h"

using namespace stx;

namespace zbase {

TermInfoMergeReducer::TermInfoMergeReducer(
    RefPtr<TermInfoTableSource> input,
    RefPtr<TermInfoTableSink> output) :
    ReportRDD(input.get(), output.get()),
    input_table_(input),
    output_table_(output) {}

void TermInfoMergeReducer::onInit() {
  input_table_->forEach(std::bind(
      &TermInfoMergeReducer::onTermInfo,
      this,
      std::placeholders::_1,
      std::placeholders::_2));
}

void TermInfoMergeReducer::onTermInfo(const String& key, const TermInfo& ti) {
  counters_[key].merge(ti);
}

void TermInfoMergeReducer::onFinish() {
  for (auto& ctr : counters_) {
    output_table_->addRow(ctr.first, ctr.second);
  }
}

} // namespace zbase

