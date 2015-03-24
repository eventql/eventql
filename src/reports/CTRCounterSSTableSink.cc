
/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "reports/CTRCounterSSTableSink.h"

using namespace fnord;

namespace cm {

CTRCounterSSTableSink::CTRCounterSSTableSink(
    const String& output_file) :
    output_file_(output_file) {}

void CTRCounterSSTableSink::onEvent(ReportEventType type, void* ev) {
  switch (type) {

    case ReportEventType::BEGIN:
      return;

    case ReportEventType::CTR_COUNTER:
      return;

    case ReportEventType::END:
      return;

    default:
      RAISE(kRuntimeError, "unknown event type");

  }
}

Set<String> CTRCounterSSTableSink::outputFiles() {
  return Set<String> { output_file_ };
}

} // namespace cm

