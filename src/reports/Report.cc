/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "reports/Report.h"

using namespace fnord;

namespace cm {

List<RefPtr<ReportSource>> Report::inputs() {
  return inputs_;
}

List<RefPtr<ReportSink>> Report::outputs() {
  return outputs_;
}

void Report::addInput(RefPtr<ReportSource> input) {
  inputs_.emplace_back(input);
}

void Report::addOutput(RefPtr<ReportSink> output) {
  outputs_.emplace_back(output);
}

} // namespace cm

