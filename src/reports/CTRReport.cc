/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "reports/CTRReport.h"

using namespace fnord;

namespace cm {

CTRReport::CTRReport(
    RefPtr<JoinedQueryTableSource> input,
    RefPtr<CTRCounterTableSink> output,
    ItemEligibility eligibility) :
    Report(input.get(), output.get()),
    joined_queries_(input),
    ctr_table_(output),
    eligibility_(eligibility) {}

void CTRReport::onInit() {
  joined_queries_->forEach(std::bind(
      &CTRReport::onJoinedQuery,
      this,
      std::placeholders::_1));
}

void CTRReport::onJoinedQuery(const JoinedQuery& q) {
  if (!isQueryEligible(eligibility_, q)) {
    return;
  }

  auto lang = languageToString(extractLanguage(q.attrs));
  auto device_type = extractDeviceType(q.attrs);
  auto test_group = extractTestGroup(q.attrs);
  auto page_type = extractPageType(q.attrs);

  uint64_t num_clicks = 0;
  for (auto& item : q.items) {
    if (!isItemEligible(eligibility_, q, item)) {
      continue;
    }

    if (item.clicked) ++num_clicks;
  }

  Set<String> keys;

  keys.emplace(StringUtil::format(
      "$0~all~$1~all",
      lang,
      device_type));

  keys.emplace(StringUtil::format(
      "$0~all~$1~$2",
      lang,
      device_type,
      page_type));

  keys.emplace(StringUtil::format(
      "$0~$1~$2~all",
      lang,
      test_group,
      device_type));

  keys.emplace(StringUtil::format(
      "$0~$1~$2~$3",
      lang,
      test_group,
      device_type,
      page_type));

  for (const auto& key : keys) {
    auto& ctr = counters_[key];
    ++ctr.num_views;
    if (num_clicks > 0) ++ctr.num_clicks;
    ctr.num_clicked += num_clicks;
  }
}

void CTRReport::onFinish() {
  for (auto& ctr : counters_) {
    ctr_table_->addRow(ctr.first, ctr.second);
  }
}


} // namespace cm

