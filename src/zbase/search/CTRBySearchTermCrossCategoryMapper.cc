/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "zbase/search/CTRBySearchTermCrossCategoryMapper.h"

using namespace stx;

namespace cm {

CTRBySearchTermCrossCategoryMapper::CTRBySearchTermCrossCategoryMapper(
    RefPtr<AnalyticsTableScanSource> input,
    RefPtr<CTRCounterTableSink> output,
    const String& category_field) :
    ReportRDD(input.get(), output.get()),
    input_(input),
    ctr_table_(output),
    field_(category_field),
    time_col_(input->tableScan()->fetchColumn("search_queries.time")),
    page_col_(input->tableScan()->fetchColumn("search_queries.page")),
    qstr_col_(input->tableScan()->fetchColumn("search_queries.query_string_normalized")),
    lang_col_(input->tableScan()->fetchColumn("search_queries.language")),
    itemid_col_(input->tableScan()->fetchColumn("search_queries.result_items.item_id")),
    clicked_col_(input->tableScan()->fetchColumn("search_queries.result_items.clicked")),
    category_col_(
        input->tableScan()->fetchColumn(category_field)) {}

void CTRBySearchTermCrossCategoryMapper::onInit() {
  input_->tableScan()->onQuery(
      std::bind(&CTRBySearchTermCrossCategoryMapper::onQuery, this));

  input_->tableScan()->onQueryItem(
      std::bind(&CTRBySearchTermCrossCategoryMapper::onQueryItem, this));
}

void CTRBySearchTermCrossCategoryMapper::onQuery() {
  cur_terms_.clear();
  cur_terms_ = StringUtil::split(qstr_col_->getString(), " ");
  cur_lang_ = languageToString((Language) lang_col_->getUInt32());
}

void CTRBySearchTermCrossCategoryMapper::onQueryItem() {
  if (cur_terms_.empty()) {
    return;
  }

  auto catid = category_col_->getUInt32();
  if (catid == 0) {
    return;
  }

  for (const auto& t : cur_terms_) {
    auto key = StringUtil::format(
        "$0~$1~$2",
        cur_lang_,
        t,
        catid);

    auto& c = counters_[key];
    ++c.num_views;

    if (clicked_col_->getBool()) {
     ++c.num_clicks;
    }
  }
}

void CTRBySearchTermCrossCategoryMapper::onFinish() {
  for (auto& ctr : counters_) {
    ctr_table_->addRow(ctr.first, ctr.second);
  }

  counters_.clear();
}


} // namespace cm

