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

Report::Report(
    RefPtr<ReportSource> input,
    RefPtr<ReportSink> output) :
    input_(input),
    output_(output),
    running(false) {}

Report::~Report() {}

void Report::onInit() {}

void Report::onFinish() {}

RefPtr<ReportSource> Report::input() {
  return input_;
}

RefPtr<ReportSink> Report::output() {
  return output_;
}

} // namespace cm

