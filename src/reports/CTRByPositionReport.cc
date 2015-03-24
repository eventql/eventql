/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "reports/CTRByPositionReport.h"

using namespace fnord;

namespace cm {

CTRByPositionReport::CTRByPositionReport(
    const String& output_file) :
    output_file_(output_file) {}

void CTRByPositionReport::onEvent(ReportEventType type, void* ev) {
  switch (type) {
    case ReportEventType::JOINED_QUERY:
      fnord::iputs("joined query! $0", ev);
      return;

    case ReportEventType::BEGIN:
    case ReportEventType::END:
      return;

    default:
      RAISE(kRuntimeError, "unknown event type");

  }
}

Set<String> CTRByPositionReport::inputFiles() {
  return Set<String>();
}

Set<String> CTRByPositionReport::outputFiles() {
  return Set<String> { output_file_ };
}

} // namespace cm

