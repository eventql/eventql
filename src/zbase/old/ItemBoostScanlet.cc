/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "ItemBoostScanlet.h"

using namespace stx;

namespace zbase {

ItemBoostScanlet::ItemBoostScanlet(
  const ItemBoostParams& params) :
  itemid_col_(scan_.fetchColumn("search_queries.result_items.item_id")),
  clicked_col_(scan_.fetchColumn("search_queries.result_items.clicked")) {
  scan_.onQueryItem(std::bind(&ItemBoostScanlet::onQueryItem, this));
}

void ItemBoostScanlet::onQueryItem() {
  auto itemid = itemid_col_->getString();
  auto clicked = clicked_col_->getBool();

  auto& ctr = counters_[itemid];
  ++ctr.num_views;
  if (clicked) ++ctr.num_clicks;
}

void ItemBoostScanlet::getResult(ItemBoostResult* result) {
  for (const auto& ctr : counters_) {
    auto ires = result->add_items();
    ires->set_item_id(ctr.first);
    ires->set_num_impressions(ctr.second.num_views);
    ires->set_num_clicks(ctr.second.num_clicks);
  }
}

void ItemBoostScanlet::mergeResults(
    ItemBoostResult* dst,
    ItemBoostResult* src) {
  HashMap<String, ItemBoostResultItem*> id_map;

  for (auto& item : *dst->mutable_items()) {
    id_map.emplace(item.item_id(), &item);
  }

  for (const auto& src_item : src->items()) {
    auto& dst_item = id_map[src_item.item_id()];
    if (dst_item == nullptr) {
      dst_item = dst->add_items();
      dst_item->set_item_id(src_item.item_id());
    }

    dst_item->set_num_impressions(
        dst_item->num_impressions() + src_item.num_impressions());

    dst_item->set_num_clicks(
        dst_item->num_clicks() + src_item.num_clicks());
  }
}

}

