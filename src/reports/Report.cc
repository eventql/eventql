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

void Report::addReport(RefPtr<Report> report) {
  children_.emplace_back(report);
}

Set<String> Report::inputFiles() {
  Set<String> files;

  for (const auto& cld : children_) {
    for (const auto& f : cld->inputFiles()) {
      files.emplace(f);
    }
  }

  return files;
}

Set<String> Report::outputFiles() {
  Set<String> files;

  for (const auto& cld : children_) {
    for (const auto& f : cld->outputFiles()) {
      files.emplace(f);
    }
  }

  return files;
}

void Report::emitEvent(
      ReportEventType type,
      ReportEventTime time,
      void* ev) {
  for (const auto& cld : children_) {
    cld->onEvent(type, time, ev);
  }
}

} // namespace cm

