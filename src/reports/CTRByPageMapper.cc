/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "reports/CTRByPageMapper.h"

using namespace fnord;

namespace cm {

CTRByPageMapper::CTRByPageMapper(
    RefPtr<JoinedQueryTableSource> input,
    RefPtr<CTRCounterTableSink> output,
    ItemEligibility eligibility) :
    Report(input.get(), output.get()),
    joined_queries_(input),
    ctr_table_(output),
    eligibility_(eligibility) {}

void CTRByPageMapper::onInit() {
  joined_queries_->forEach(std::bind(
      &CTRByPageMapper::onJoinedQuery,
      this,
      std::placeholders::_1));
}

void CTRByPageMapper::onJoinedQuery(const JoinedQuery& q) {
  if (!isQueryEligible(eligibility_, q)) {
    return;
  }

  auto pg_str = cm::extractAttr(q.attrs, "pg");
  if (pg_str.isEmpty()) {
    return;
  }

  auto pg = std::stoul(pg_str.get());

  auto lang = languageToString(extractLanguage(q.attrs));
  auto device_type = extractDeviceType(q.attrs);
  auto test_group = extractTestGroup(q.attrs);

  size_t num_clicks = 0;
  for (auto& item : q.items) {
    if (!isItemEligible(eligibility_, q, item)) {
      continue;
    }

    if (item.clicked) {
      num_clicks++;
    }
  }

  Set<String> keys;
  keys.emplace(StringUtil::format(
      "$0~$1~$2~$3",
      lang,
      test_group,
      device_type,
      pg));

  keys.emplace(StringUtil::format(
      "$0~all~$1~$2",
      lang,
      device_type,
      pg));

  for (const auto& key : keys) {
    auto& ctr = counters_[key];
    ++ctr.num_views;
    ctr.num_clicks += num_clicks > 0 ? 1 : 0;
    ctr.num_clicked += num_clicks;
  }
}

void CTRByPageMapper::onFinish() {
  for (auto& ctr : counters_) {
    ctr_table_->addRow(ctr.first, ctr.second);
  }
}


} // namespace cm

