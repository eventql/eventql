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

void CTRByPositionReport::onEvent(ReportEventType type, void* ev) {
  switch (type) {
    case ReportEventType::JOINED_QUERY:
      onJoinedQuery(*((JoinedQuery*) ev));
      return;

    case ReportEventType::BEGIN:
      return;

    case ReportEventType::END:
      flushResults();
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

    auto key = StringUtil::format(
        "$0~$1~$2~$3",
        lang,
        device_type,
        test_group,
        item.position);

    fnord::iputs("key: $0 ---- $1", key, q.attrs);
    auto& ctr = counters_[key];
    ++ctr.num_views;
    ctr.num_clicks += item.clicked;
  }
}

void CTRByPositionReport::flushResults() {

}

} // namespace cm

