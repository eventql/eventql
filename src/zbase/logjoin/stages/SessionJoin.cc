/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "zbase/logjoin/stages/SessionJoin.h"
#include "zbase/logjoin/common.h"

using namespace stx;

namespace zbase {

void SessionJoin::process(RefPtr<SessionContext> ctx) {

  /* load builtin events into structured format */
  std::vector<TrackedQuery> queries;
  std::vector<TrackedItemVisit> page_views;
  std::vector<TrackedCartItem> cart_items;

  for (const auto& ev : ctx->events) {
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
  uint32_t num_cart_items = 0;
  uint32_t num_order_items = 0;
  uint32_t gmv_eurcents = 0;
  uint32_t cart_value_eurcents = 0;
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

  for (const auto& ci : cart_items) {
    auto& ciobj = ctx->addOutputEvent(
        ci.time,
        SHA1::compute(ctx->uuid + "~" + ci.item.docID().docid),
        "cart_items")->obj;

    ciobj.addField("time", StringUtil::toString(ci.time.unixMicros()));
    ciobj.addField("item_id", ci.item.docID().docid);
    ciobj.addField("quantity", StringUtil::toString(ci.quantity));
    ciobj.addField("price_cents", StringUtil::toString(ci.price_cents));
    ciobj.addUInt32Field("currency", (uint32_t) currencyFromString(ci.currency));
    ciobj.addField("checkout_step", StringUtil::toString(ci.checkout_step));
  }

  for (const auto& q : queries) {
    auto& qobj = ctx->addOutputEvent(
        q.time,
        SHA1::compute(ctx->uuid + "~" + q.clickid),
        "search_query")->obj;

    qobj.addField("time", StringUtil::toString(q.time.unixMicros()));
    qobj.addUInt32Field("language", (uint32_t) zbase::extractLanguage(q.attrs));

    auto qstr = zbase::extractQueryString(q.attrs);
    if (!qstr.isEmpty()) {
      qobj.addStringField("query_string", qstr.get());
    }

    auto slrid = zbase::extractAttr(q.attrs, "slrid");
    if (!slrid.isEmpty()) {
      qobj.addField("shop_id", slrid.get());
    }

    qobj.addUInt32Field("num_result_items", q.nitems);
    qobj.addUInt32Field("num_result_items_clicked", q.nclicks);
    qobj.addUInt32Field("num_ad_impressions", q.nads);
    qobj.addUInt32Field("num_ad_clicks", q.nadclicks);
    qobj.addUInt32Field("num_cart_items", q.num_cart_items);
    qobj.addUInt32Field("cart_value_eurcents", q.cart_value_eurcents);
    qobj.addUInt32Field("num_order_items", q.num_order_items);
    qobj.addUInt32Field("gmv_eurcents", q.gmv_eurcents);

    auto pg_str = zbase::extractAttr(q.attrs, "pg");
    if (!pg_str.isEmpty()) {
      qobj.addField("page", pg_str.get());
    }

    auto abgrp = zbase::extractABTestGroup(q.attrs);
    if (!abgrp.isEmpty()) {
      qobj.addField("ab_test_group", StringUtil::toString(abgrp.get()));
    }

    auto qcat1 = zbase::extractAttr(q.attrs, "q_cat1");
    if (!qcat1.isEmpty()) {
      qobj.addField("category1", qcat1.get());
    }

    auto qcat2 = zbase::extractAttr(q.attrs, "q_cat2");
    if (!qcat2.isEmpty()) {
      qobj.addField("category2", qcat2.get());
    }

    auto qcat3 = zbase::extractAttr(q.attrs, "q_cat3");
    if (!qcat3.isEmpty()) {
      qobj.addField("category3", qcat3.get());
    }

    auto page_type = extractPageType(q.attrs);
    String query_type = pageTypeToString((PageType) page_type);
    auto qtype_attr = zbase::extractAttr(q.attrs, "qt");
    if (!qtype_attr.isEmpty()) {
      query_type = qtype_attr.get();
    }

    qobj.addUInt32Field("device_type", (uint32_t) extractDeviceType(q.attrs));
    qobj.addUInt32Field("page_type", (uint32_t) page_type);
    qobj.addField("query_type", query_type);

    for (const auto& item : q.items) {
      qobj.addObject("result_items", [&item] (msg::DynamicMessage* ev) {
        ev->addField("position", StringUtil::toString(item.position));
        ev->addField("item_id", item.item.docID().docid);
        ev->addBoolField("clicked", item.clicked);
        ev->addBoolField("seen", item.seen);
      });
    }
  }

  for (const auto& iv : page_views) {
    auto& ivobj = ctx->addOutputEvent(
        iv.time,
        SHA1::compute(ctx->uuid + "~" + iv.clickid),
        "page_view")->obj;

    ivobj.addField("time", StringUtil::toString(iv.time.unixMicros()));
    ivobj.addField("item_id", iv.item.docID().docid);
  }

  ctx->setAttribute("num_cart_items", StringUtil::toString(num_cart_items));
  ctx->setAttribute("cart_value_eurcents", StringUtil::toString(cart_value_eurcents));
  ctx->setAttribute("num_order_items", StringUtil::toString(num_order_items));
  ctx->setAttribute("gmv_eurcents", StringUtil::toString(gmv_eurcents));
}

void SessionJoin::processSearchQueryEvent(
    const TrackedEvent& event,
    Vector<TrackedQuery>* queries) {
  TrackedQuery query;
  query.time = event.time;
  query.clickid = event.evid;

  URI::ParamList logline;
  URI::parseQueryString(event.data, &logline);
  query.fromParams(logline);

  for (auto& q : *queries) {
    if (q.clickid == query.clickid) {
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
  visit.clickid = event.evid;

  URI::ParamList logline;
  URI::parseQueryString(event.data, &logline);
  visit.fromParams(logline);

  for (auto& v : *page_views) {
    if (v.clickid == visit.clickid) {
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

} // namespace zbase

