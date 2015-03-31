/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "reports/CTRByQueryAttributeMapper.h"

using namespace fnord;

namespace cm {

CTRByQueryAttributeMapper::CTRByQueryAttributeMapper(
    RefPtr<JoinedQueryTableSource> input,
    RefPtr<CTRCounterTableSink> output,
    const String& attribute,
    ItemEligibility eligibility) :
    Report(input.get(), output.get()),
    joined_queries_(input),
    ctr_table_(output),
    attribute_(attribute),
    eligibility_(eligibility) {}

void CTRByQueryAttributeMapper::onInit() {
  joined_queries_->forEach(std::bind(
      &CTRByQueryAttributeMapper::onJoinedQuery,
      this,
      std::placeholders::_1));
}

void CTRByQueryAttributeMapper::onJoinedQuery(const JoinedQuery& q) {
  if (!isQueryEligible(eligibility_, q)) {
    return;
  }

  auto attr = cm::extractAttr(q.attrs, attribute_);
  if (attr.isEmpty()) {
    return;
  }

  size_t num_clicks = 0;
  for (auto& item : q.items) {
    if (!isItemEligible(eligibility_, q, item)) {
      continue;
    }

    if (item.clicked) {
      num_clicks++;
    }
  }

  auto& ctr = counters_[attr.get()];
  auto& global_ctr = counters_["__all"];
  ++ctr.num_views;
  ++global_ctr.num_views;
  ctr.num_clicks += num_clicks > 0 ? 1 : 0;
  global_ctr.num_clicks += num_clicks > 0 ? 1 : 0;
  ctr.num_clicked += num_clicks;
  global_ctr.num_clicked += num_clicks;
}

void CTRByQueryAttributeMapper::onFinish() {
  for (auto& ctr : counters_) {
    ctr_table_->addRow(ctr.first, ctr.second);
  }
}


} // namespace cm

