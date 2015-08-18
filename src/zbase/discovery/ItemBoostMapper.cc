/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "zbase/discovery/ItemBoostMapper.h"

using namespace stx;

namespace cm {

ItemBoostMapper::ItemBoostMapper(
    RefPtr<AnalyticsTableScanSource> input,
    RefPtr<ProtoSSTableSink<ItemBoostRow>> output) :
    ReportRDD(input.get(), output.get()),
    input_(input),
    output_(output),
    qstr_col_(input->tableScan()->fetchColumn("search_queries.query_string_normalized")),
    itemid_col_(input->tableScan()->fetchColumn("search_queries.result_items.item_id")),
    pos_col_(input->tableScan()->fetchColumn("search_queries.result_items.position")),
    clicked_col_(input->tableScan()->fetchColumn("search_queries.result_items.clicked")) {
  //posi_norm_.emplace(1 , 1.0 / 0.006592);
  //posi_norm_.emplace(2 , 1.0 / 0.006428);
  //posi_norm_.emplace(3 , 1.0 / 0.006280);
  //posi_norm_.emplace(4 , 1.0 / 0.004807);
  //posi_norm_.emplace(5 , 1.0 / 0.014855);
  //posi_norm_.emplace(6 , 1.0 / 0.011013);
  //posi_norm_.emplace(7 , 1.0 / 0.009650);
  //posi_norm_.emplace(8 , 1.0 / 0.009113);
  //posi_norm_.emplace(9 , 1.0 / 0.007431);
  //posi_norm_.emplace(10, 1.0 / 0.007575);
  //posi_norm_.emplace(11, 1.0 / 0.006746);
  //posi_norm_.emplace(12, 1.0 / 0.006797);
  //posi_norm_.emplace(13, 1.0 / 0.006326);
  //posi_norm_.emplace(14, 1.0 / 0.006120);
  //posi_norm_.emplace(15, 1.0 / 0.005872);
  //posi_norm_.emplace(16, 1.0 / 0.005912);
  //posi_norm_.emplace(17, 1.0 / 0.005468);
  //posi_norm_.emplace(18, 1.0 / 0.005541);
  //posi_norm_.emplace(19, 1.0 / 0.005432);
  //posi_norm_.emplace(20, 1.0 / 0.005267);
  //posi_norm_.emplace(21, 1.0 / 0.005092);
  //posi_norm_.emplace(22, 1.0 / 0.005211);
  //posi_norm_.emplace(23, 1.0 / 0.004906);
  //posi_norm_.emplace(24, 1.0 / 0.004943);
  //posi_norm_.emplace(25, 1.0 / 0.004797);
  //posi_norm_.emplace(26, 1.0 / 0.004828);
  //posi_norm_.emplace(27, 1.0 / 0.004719);
  //posi_norm_.emplace(28, 1.0 / 0.004761);
  //posi_norm_.emplace(29, 1.0 / 0.004475);
  //posi_norm_.emplace(30, 1.0 / 0.004629);
  //posi_norm_.emplace(31, 1.0 / 0.004537);
  //posi_norm_.emplace(32, 1.0 / 0.004463);
  //posi_norm_.emplace(33, 1.0 / 0.004437);
  //posi_norm_.emplace(34, 1.0 / 0.004544);
  //posi_norm_.emplace(35, 1.0 / 0.004429);
  //posi_norm_.emplace(36, 1.0 / 0.004562);
  //posi_norm_.emplace(37, 1.0 / 0.004946);
  //posi_norm_.emplace(38, 1.0 / 0.005049);
  //posi_norm_.emplace(39, 1.0 / 0.005191);
  //posi_norm_.emplace(40, 1.0 / 0.005817);
  //posi_norm_.emplace(41, 1.0 / 0.006957);
  //posi_norm_.emplace(42, 1.0 / 0.007209);
  //posi_norm_.emplace(43, 1.0 / 0.007507);
  //posi_norm_.emplace(44, 1.0 / 0.007008);
}

void ItemBoostMapper::onInit() {
  input_->tableScan()->onQuery(
      std::bind(&ItemBoostMapper::onQuery, this));

  input_->tableScan()->onQueryItem(
      std::bind(&ItemBoostMapper::onQueryItem, this));
}

void ItemBoostMapper::onQuery() {
  cur_terms_ = StringUtil::split(qstr_col_->getString(), " ");
}

void ItemBoostMapper::onQueryItem() {
  auto itemid = itemid_col_->getString();
  auto clicked = clicked_col_->getBool();
  auto pos = pos_col_->getUInt32();

  auto& scratch = scratch_[itemid];
  scratch.num_views += 1;
  scratch.num_clicks += clicked ? 1 : 0;

  if (!clicked) {
    return;
  }

  for (const auto& t : cur_terms_) {
    scratch.terms[t] += 1;
  }
}

void ItemBoostMapper::onFinish() {
  for (auto& s : scratch_) {
    ItemBoostRow row;
    row.set_num_impressions(s.second.num_views);
    row.set_num_clicks(s.second.num_clicks);

    for (const auto& t : s.second.terms) {
      auto term = row.add_top_terms();
      term->set_term(t.first);
      term->set_score(t.second);
    }

    output_->addRow(s.first, row);
  }
}

} // namespace cm

