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
    ItemEligibility eligibility) :
    eligibility_(eligibility) {}

void CTRByPositionReport::onEvent(
    ReportEventType type,
    ReportEventTime time,
    void* ev) {
  switch (type) {

    case ReportEventType::BEGIN:
      emitEvent(type, time, ev);
      return;

    case ReportEventType::JOINED_QUERY:
      onJoinedQuery(*((JoinedQuery*) ev));
      return;

    case ReportEventType::END:
      flushResults();
      emitEvent(type, time, ev);
      return;

    default:
      RAISE(kRuntimeError, "unknown event type");

  }
}

void CTRByPositionReport::onJoinedQuery(const JoinedQuery& q) {
  if (!isQueryEligible(eligibility_, q)) {
    return;
  }

  auto lang = languageToString(extractLanguage(q.attrs));
  auto device_type = extractDeviceType(q.attrs);
  auto test_group = extractTestGroup(q.attrs);
  for (auto& item : q.items) {
    if (!isItemEligible(eligibility_, q, item) || item.position < 1) {
      continue;
    }

    Set<String> keys;

    keys.emplace(StringUtil::format(
        "$0~$1~$2~$3",
        lang,
        test_group,
        device_type,
        item.position));

    keys.emplace(StringUtil::format(
        "$0~all~$1~$2",
        lang,
        device_type,
        item.position));

    for (const auto& key : keys) {
      auto& ctr = counters_[key];
      ++ctr.num_views;
      ctr.num_clicks += item.clicked;
    }
  }
}

void CTRByPositionReport::flushResults() {
  for (auto& ctr : counters_) {
    emitEvent(ReportEventType::CTR_COUNTER, nullptr, &ctr);
  }
}

} // namespace cm

