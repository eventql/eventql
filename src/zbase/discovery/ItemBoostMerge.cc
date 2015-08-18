/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "ItemBoostMerge.h"

using namespace stx;

namespace cm {

void ItemBoostMerge::merge(ItemBoostRow* dst, const ItemBoostRow& src) {
  dst->set_num_impressions(dst->num_impressions() + src.num_impressions());
  dst->set_num_clicks(dst->num_clicks() + src.num_clicks());

  HashMap<String, ItemBoostTermInfo*> ti;
  for (auto& t : *dst->mutable_top_terms()) {
    ti.emplace(t.term(), &t);
  }

  for (const auto& st : src.top_terms()) {
    auto& dt = ti[st.term()];
    if (dt == nullptr) {
      dt = dst->add_top_terms();
      dt->set_term(st.term());
    }

    dt->set_score(dt->score() + st.score());
  }
}

} // namespace cm
