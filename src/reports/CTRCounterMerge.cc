/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "reports/CTRCounterMerge.h"

using namespace fnord;

namespace cm {

void CTRCounterMerge::onEvent(
    ReportEventType type,
    ReportEventTime time,
    void* ev) {
  switch (type) {

    case ReportEventType::BEGIN:
      emitEvent(type, time, ev);
      return;

    case ReportEventType::JOINED_QUERY:
      onCounter(*((CTRCounter*) ev));
      return;

    case ReportEventType::END:
      flushResults();
      emitEvent(type, time, ev);
      return;

    default:
      RAISE(kRuntimeError, "unknown event type");

  }
}

void CTRCounterMerge::onCounter(const CTRCounter& c) {
  fnord::iputs("ctrcounter: $0", c.first);
  //auto& ctr = counters_[key];
}

void CTRCounterMerge::flushResults() {
  //for (auto& ctr : counters_) {
  //  emitEvent(ReportEventType::CTR_COUNTER, &ctr);
  //}
}

} // namespace cm

