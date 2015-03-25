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
    RefPtr<JoinedQueryTableSource> input,
    RefPtr<CTRCounterSSTableSink> output,
    ItemEligibility eligibility) :
    Report(input.get(), output.get()),
    joined_queries_(input),
    ctr_table_(output),
    eligibility_(eligibility) {}

void CTRByPositionReport::onInit() {
  joined_queries_->forEach(std::bind(
      &CTRByPositionReport::onJoinedQuery,
      this,
      std::placeholders::_1));
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

void CTRByPositionReport::onFinish() {
  for (auto& ctr : counters_) {
    fnord::iputs("ctr: $0", ctr.first);
//    emitEvent(ReportEventType::CTR_COUNTER, nullptr, &ctr);
  }
}


} // namespace cm

