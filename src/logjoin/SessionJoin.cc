/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "logjoin/SessionJoin.h"
#include "logjoin/common.h"

using namespace stx;

namespace cm {

void SessionJoin::process(RefPtr<TrackedSessionContext> ctx) {

  /* load builtin events into structured format */
  std::vector<TrackedQuery> queries;
  std::vector<TrackedItemVisit> page_views;
  std::vector<TrackedCartItem> cart_items;

  for (const auto& ev : ctx->tracked_session.events) {
    if (ev.evtype == "_search_query") {
      processSearchQueryEvent(ev, &queries);
      continue;
    }

    if (ev.evtype == "_pageview") {
      processPageViewEvent(ev, &page_views);
      continue;
    }

    if (ev.evtype == "_cart_items") {
      processCartItemsEvent(ev, &cart_items);
      continue;
    }
  }

  /* update queries (mark items as clicked) */
  for (auto& cur_query : queries) {

    /* search for matching item visits */
    for (auto& cur_visit : page_views) {
      auto cutoff = cur_query.time.unixMicros() +
          kMaxQueryClickDelaySeconds * kMicrosPerSecond;

      if (cur_visit.time < cur_query.time ||
          cur_visit.time.unixMicros() > cutoff) {
        continue;
      }

      for (auto& qitem : cur_query.items) {
        if (cur_visit.item == qitem.item) {
          qitem.clicked = true;
          qitem.seen = true;
          break;
        }
      }
    }
  }

  /* calculate global gmv */
  uint32_t num_cart_items;
  uint32_t num_order_items;
  uint32_t gmv_eurcents;
  uint32_t cart_value_eurcents;
  HashMap<String, uint64_t> cart_eurcents_per_item;
  HashMap<String, uint64_t> gmv_eurcents_per_item;
  for (const auto& ci : cart_items) {
    auto currency = currencyFromString(ci.currency);
    auto eur = cconv()->convert(Money(ci.price_cents, currency), CURRENCY_EUR);
    auto eurcents = eur.cents;
    eurcents *= ci.quantity;
    cart_eurcents_per_item.emplace(ci.item.docID().docid, eurcents);

    ++num_cart_items;
    cart_value_eurcents += eurcents;
    if (ci.checkout_step == 1) {
      gmv_eurcents_per_item.emplace(ci.item.docID().docid, eurcents);
      ++num_order_items;
      gmv_eurcents += eurcents;
    }
  }

  /* calculate gmv and ctrs per query */
  for (auto& q : queries) {
    auto slrid = extractAttr(q.attrs, "slrid");

    for (auto& i : q.items) {
      // DAWANDA HACK
      if (i.position >= 1 && i.position <= 4 && slrid.isEmpty()) {
        ++q.nads;
        q.nadclicks += i.clicked;
      }
      // EOF DAWANDA HACK

      ++q.nitems;

      if (i.clicked) {
        ++q.nclicks;

        {
          auto ci = cart_eurcents_per_item.find(i.item.docID().docid);
          if (ci != cart_eurcents_per_item.end()) {
            ++q.num_cart_items;
            q.cart_value_eurcents += ci->second;
          }
        }

        {
          auto ci = gmv_eurcents_per_item.find(i.item.docID().docid);
          if (ci != gmv_eurcents_per_item.end()) {
            ++q.num_order_items;
            q.gmv_eurcents += ci->second;
          }
        }
      }
    }
  }

}

void SessionJoin::processSearchQueryEvent(
    const TrackedEvent& event,
    Vector<TrackedQuery>* queries) {
  TrackedQuery query;
  query.time = event.time;
  query.eid = event.evid;

  URI::ParamList logline;
  URI::parseQueryString(event.data, &logline);
  query.fromParams(logline);

  for (auto& q : *queries) {
    if (q.eid == query.eid) {
      q.merge(query);
      return;
    }
  }

  queries->emplace_back(query);
}

void SessionJoin::processPageViewEvent(
    const TrackedEvent& event,
    Vector<TrackedItemVisit>* page_views) {
  TrackedItemVisit visit;
  visit.time = event.time;
  visit.eid = event.evid;

  URI::ParamList logline;
  URI::parseQueryString(event.data, &logline);
  visit.fromParams(logline);

  for (auto& v : *page_views) {
    if (v.eid == visit.eid) {
      v.merge(visit);
      return;
    }
  }

  page_views->emplace_back(visit);
}

void SessionJoin::processCartItemsEvent(
    const TrackedEvent& event,
    Vector<TrackedCartItem>* cart_items) {
  URI::ParamList logline;
  URI::parseQueryString(event.data, &logline);

  auto new_cart_items = TrackedCartItem::fromParams(logline);
  for (auto& ci : new_cart_items) {
    ci.time = event.time;
  }

  for (const auto& cart_item : new_cart_items) {
    bool merged = false;

    for (auto& c : *cart_items) {
      if (c.item == cart_item.item) {
        c.merge(cart_item);
        merged = true;
        break;
      }
    }

    if (!merged) {
      cart_items->emplace_back(cart_item);
    }
  }
}

} // namespace cm

