/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "reports/TermInfoMerge.h"

using namespace fnord;

namespace cm {

TermInfoMerge::TermInfoMerge(
    RefPtr<TermInfoTableSource> input,
    RefPtr<TermInfoTableSink> output) :
    Report(input.get(), output.get()),
    input_table_(input),
    output_table_(output) {}

void TermInfoMerge::onInit() {
  input_table_->forEach(std::bind(
      &TermInfoMerge::onTermInfo,
      this,
      std::placeholders::_1,
      std::placeholders::_2));
}

void TermInfoMerge::onTermInfo(const String& key, const TermInfo& ti) {
  //counters_[key].merge(c);

}

void TermInfoMerge::onFinish() {
  for (auto& ctr : counters_) {
    output_table_->addRow(ctr.first, ctr.second);
  }
}

} // namespace cm

