/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "fnord-msg/MessageBuilder.h"
#include "fnord-msg/MessageObject.h"
#include "fnord-msg/MessageEncoder.h"
#include "fnord-msg/MessageDecoder.h"
#include "fnord-msg/MessagePrinter.h"
#include <fnord-fts/fts.h>
#include <fnord-fts/fts_common.h>
#include "logjoin/LogJoinTarget.h"
#include "common.h"

using namespace fnord;

namespace cm {

LogJoinTarget::LogJoinTarget(
    const msg::MessageSchema& joined_sessions_schema,
    bool dry_run) :
    joined_sessions_schema_(joined_sessions_schema),
    dry_run_(dry_run),
    num_sessions(0),
    cconv_(currencyConversionTable()) {}

void LogJoinTarget::setNormalize(
    Function<fnord::String (Language lang, const fnord::String& query)> normalizeCb) {
  normalize_ = normalizeCb;
}

void LogJoinTarget::setGetField(
    Function<Option<String> (const DocID& docid, const String& feature)> getFieldCb) {
  get_field_ = getFieldCb;
}

Buffer LogJoinTarget::trackedSessionToJoinedSession(TrackedSession& session) {
  if (!get_field_) {
    RAISE(kRuntimeError, "getField has not been initialized");
  }

  if (!normalize_) {
    RAISE(kRuntimeError, "normalize has not been initialized");
  }

  session.joinEvents(cconv_);
  const auto& schema = joined_sessions_schema_;
  msg::MessageObject obj;

  for (const auto& ci : session.cart_items) {
    auto& ci_obj = obj.addChild(schema.id("cart_items"));

    ci_obj.addChild(
        schema.id("cart_items.time"),
        (uint32_t) (ci.time.unixMicros() / kMicrosPerSecond));

    ci_obj.addChild(
        schema.id("cart_items.item_id"),
        ci.item.docID().docid);

    ci_obj.addChild(
        schema.id("cart_items.quantity"),
        ci.quantity);

    ci_obj.addChild(
        schema.id("cart_items.price_cents"),
        ci.price_cents);

    ci_obj.addChild(
        schema.id("cart_items.currency"),
        (uint32_t) currencyFromString(ci.currency));

    ci_obj.addChild(
        schema.id("cart_items.checkout_step"),
        ci.checkout_step);

    // FIXPAUL use getFields...
    auto docid = ci.item.docID();
    auto shopid = get_field_(docid, "shop_id");
    if (shopid.isEmpty()) {
      fnord::logWarning(
          "cm.logjoin",
          "item not found in featureindex: $0",
          docid.docid);
    } else {
      ci_obj.addChild(
          schema.id("cart_items.shop_id"),
          (uint32_t) std::stoull(shopid.get()));
    }

    auto category1 = get_field_(docid, "category1");
    if (!category1.isEmpty()) {
      ci_obj.addChild(
          schema.id("cart_items.category1"),
          (uint32_t) std::stoull(category1.get()));
    }

    auto category2 = get_field_(docid, "category2");
    if (!category2.isEmpty()) {
      ci_obj.addChild(
          schema.id("cart_items.category2"),
          (uint32_t) std::stoull(category2.get()));
    }

    auto category3 = get_field_(docid, "category3");
    if (!category3.isEmpty()) {
      ci_obj.addChild(
          schema.id("cart_items.category3"),
          (uint32_t) std::stoull(category3.get()));
    }
  }

  uint32_t sess_abgrp = 0;
  for (const auto& q : session.queries) {
    auto& qry_obj = obj.addChild(schema.id("search_queries"));

    /* queries.time */
    qry_obj.addChild(
        schema.id("search_queries.time"),
        (uint32_t) (q.time.unixMicros() / kMicrosPerSecond));

    /* queries.language */
    auto lang = cm::extractLanguage(q.attrs);
    qry_obj.addChild(schema.id("search_queries.language"), (uint32_t) lang);

    /* queries.query_string */
    auto qstr = cm::extractQueryString(q.attrs);
    if (!qstr.isEmpty()) {
      auto qstr_norm = normalize_(lang, qstr.get());
      qry_obj.addChild(schema.id("search_queries.query_string"), qstr.get());
      qry_obj.addChild(schema.id("search_queries.query_string_normalized"), qstr_norm);
    }

    /* queries.shopid */
    auto slrid = cm::extractAttr(q.attrs, "slrid");
    if (!slrid.isEmpty()) {
      uint32_t sid = std::stoul(slrid.get());
      qry_obj.addChild(schema.id("search_queries.shop_id"), sid);
    }

    qry_obj.addChild(schema.id("search_queries.num_result_items"), q.nitems);
    qry_obj.addChild(schema.id("search_queries.num_result_items_clicked"), q.nclicks);
    qry_obj.addChild(schema.id("search_queries.num_ad_impressions"), q.nads);
    qry_obj.addChild(schema.id("search_queries.num_ad_clicks"), q.nadclicks);
    qry_obj.addChild(schema.id("search_queries.num_cart_items"), q.num_cart_items);
    qry_obj.addChild(schema.id("search_queries.cart_value_eurcents"), q.cart_value_eurcents);
    qry_obj.addChild(schema.id("search_queries.num_order_items"), q.num_order_items);
    qry_obj.addChild(schema.id("search_queries.gmv_eurcents"), q.gmv_eurcents);

    /* queries.page */
    auto pg_str = cm::extractAttr(q.attrs, "pg");
    if (!pg_str.isEmpty()) {
      uint32_t pg = std::stoul(pg_str.get());
      qry_obj.addChild(schema.id("search_queries.page"), pg);
    }

    /* queries.ab_test_group */
    auto abgrp = cm::extractABTestGroup(q.attrs);
    if (!abgrp.isEmpty()) {
      sess_abgrp = abgrp.get();
      qry_obj.addChild(schema.id("search_queries.ab_test_group"), abgrp.get());
    }

    /* queries.experiments */
    auto qexps = q.joinedExperiments();
    if (qexps.size() > 0) {
      qry_obj.addChild(schema.id("search_queries.experiments"), qexps);
    }

    /* queries.category1 */
    auto qcat1 = cm::extractAttr(q.attrs, "q_cat1");
    if (!qcat1.isEmpty()) {
      uint32_t c = std::stoul(qcat1.get());
      qry_obj.addChild(schema.id("search_queries.category1"), c);
    }

    /* queries.category1 */
    auto qcat2 = cm::extractAttr(q.attrs, "q_cat2");
    if (!qcat2.isEmpty()) {
      uint32_t c = std::stoul(qcat2.get());
      qry_obj.addChild(schema.id("search_queries.category2"), c);
    }

    /* queries.category1 */
    auto qcat3 = cm::extractAttr(q.attrs, "q_cat3");
    if (!qcat3.isEmpty()) {
      uint32_t c = std::stoul(qcat3.get());
      qry_obj.addChild(schema.id("search_queries.category3"), c);
    }

    /* queries.device_type */
    qry_obj.addChild(
        schema.id("search_queries.device_type"),
        (uint32_t) extractDeviceType(q.attrs));

    /* queries.page_type */
    auto page_type = extractPageType(q.attrs);
    qry_obj.addChild(
        schema.id("search_queries.page_type"),
        (uint32_t) page_type);

    /* queries.query_type */
    String query_type = pageTypeToString(page_type);
    auto qtype_attr = cm::extractAttr(q.attrs, "qt");
    if (!qtype_attr.isEmpty()) {
      query_type = qtype_attr.get();
    }
    qry_obj.addChild(
        schema.id("search_queries.query_type"),
        query_type);

    for (const auto& item : q.items) {
      auto& item_obj = qry_obj.addChild(
          schema.id("search_queries.result_items"));

      item_obj.addChild(
          schema.id("search_queries.result_items.position"),
          (uint32_t) item.position);

      item_obj.addChild(
          schema.id("search_queries.result_items.item_id"),
          item.item.docID().docid);

      if (item.clicked) {
        item_obj.addChild(
            schema.id("search_queries.result_items.clicked"),
            msg::TRUE);
      } else {
        item_obj.addChild(
            schema.id("search_queries.result_items.clicked"),
            msg::FALSE);
      }

      if (item.seen) {
        item_obj.addChild(
            schema.id("search_queries.result_items.seen"),
            msg::TRUE);
      } else {
        item_obj.addChild(
            schema.id("search_queries.result_items.seen"),
            msg::FALSE);
      }

      auto docid = item.item.docID();

      auto shopid = get_field_(docid, "shop_id");
      if (shopid.isEmpty()) {
        fnord::logWarning(
            "cm.logjoin",
            "item not found in featureindex: $0",
            docid.docid);
      } else {
        item_obj.addChild(
            schema.id("search_queries.result_items.shop_id"),
            (uint32_t) std::stoull(shopid.get()));
      }

      auto category1 = get_field_(docid, "category1");
      if (!category1.isEmpty()) {
        item_obj.addChild(
            schema.id("search_queries.result_items.category1"),
            (uint32_t) std::stoull(category1.get()));
      }

      auto category2 = get_field_(docid, "category2");
      if (!category2.isEmpty()) {
        item_obj.addChild(
            schema.id("search_queries.result_items.category2"),
            (uint32_t) std::stoull(category2.get()));
      }

      auto category3 = get_field_(docid, "category3");
      if (!category3.isEmpty()) {
        item_obj.addChild(
            schema.id("search_queries.result_items.category3"),
            (uint32_t) std::stoull(category3.get()));
      }

      /* DAWANDA HACK */
      if (item.position <= 4 && slrid.isEmpty()) {
        item_obj.addChild(
            schema.id("search_queries.result_items.is_paid_result"),
            msg::TRUE);
      }

      if ((item.position > 40 && slrid.isEmpty()) ||
          StringUtil::beginsWith(query_type, "recos_")) {
        item_obj.addChild(
            schema.id("search_queries.result_items.is_recommendation"),
            msg::TRUE);
      }
      /* EOF DAWANDA HACK */
    }
  }

  for (const auto& iv : session.item_visits) {
    auto& iv_obj = obj.addChild(schema.id("item_visits"));

    iv_obj.addChild(
        schema.id("item_visits.time"),
        (uint32_t) (iv.time.unixMicros() / kMicrosPerSecond));

    iv_obj.addChild(
        schema.id("item_visits.item_id"),
        iv.item.docID().docid);

    auto docid = iv.item.docID();
    auto shopid = get_field_(docid, "shop_id");
    if (shopid.isEmpty()) {
      fnord::logWarning(
          "cm.logjoin",
          "item not found in featureindex: $0",
          docid.docid);
    } else {
      iv_obj.addChild(
          schema.id("item_visits.shop_id"),
          (uint32_t) std::stoull(shopid.get()));
    }

    auto category1 = get_field_(docid, "category1");
    if (!category1.isEmpty()) {
      iv_obj.addChild(
          schema.id("item_visits.category1"),
          (uint32_t) std::stoull(category1.get()));
    }

    auto category2 = get_field_(docid, "category2");
    if (!category2.isEmpty()) {
      iv_obj.addChild(
          schema.id("item_visits.category2"),
          (uint32_t) std::stoull(category2.get()));
    }

    auto category3 = get_field_(docid, "category3");
    if (!category3.isEmpty()) {
      iv_obj.addChild(
          schema.id("item_visits.category3"),
          (uint32_t) std::stoull(category3.get()));
    }
  }

  if (sess_abgrp > 0) {
    obj.addChild(schema.id("ab_test_group"), sess_abgrp);
  }

  auto exps = session.joinedExperiments();
  if (exps.size() > 0) {
    obj.addChild(schema.id("experiments"), exps);
  }

  if (!session.referrer_url.isEmpty()) {
    obj.addChild(schema.id("referrer_url"), session.referrer_url.get());
  }

  if (!session.referrer_campaign.isEmpty()) {
    obj.addChild(schema.id("referrer_campaign"), session.referrer_campaign.get());
  }

  if (!session.referrer_name.isEmpty()) {
    obj.addChild(schema.id("referrer_name"), session.referrer_name.get());
  }

  if (!session.customer_session_id.isEmpty()) {
    obj.addChild(schema.id("customer_session_id"), session.customer_session_id.get());
  }

  obj.addChild(schema.id("num_cart_items"), session.num_cart_items);
  obj.addChild(schema.id("cart_value_eurcents"), session.cart_value_eurcents);
  obj.addChild(schema.id("num_order_items"), session.num_order_items);
  obj.addChild(schema.id("gmv_eurcents"), session.gmv_eurcents);
  obj.addChild(schema.id("customer"), session.customer_key);

  Buffer msg_buf;
  auto first_seen = session.firstSeenTime();
  auto last_seen = session.lastSeenTime();
  if (first_seen.isEmpty() || last_seen.isEmpty()) {
    RAISE(kRuntimeError, "session: time isn't set");
  }

  obj.addChild(
      schema.id("first_seen_time"),
      first_seen.get().unixMicros() / kMicrosPerSecond);

  obj.addChild(
      schema.id("last_seen_time"),
      last_seen.get().unixMicros() / kMicrosPerSecond);

  msg::MessageEncoder::encode(obj, joined_sessions_schema_, &msg_buf);
  return msg_buf;
}


void LogJoinTarget::onSession(
    mdb::MDBTransaction* txn,
    TrackedSession& session) {

  Buffer msg_buf = trackedSessionToJoinedSession(session);
  if (dry_run_) {
    fnord::logInfo(
        "cm.logjoin",
        "[DRYRUN] not uploading session: ", 1);
        //msg::MessagePrinter::print(obj, joined_sessions_schema_));
  } else {
    auto first_seen = session.firstSeenTime();
    auto time = first_seen.get().unixMicros();
    util::BinaryMessageWriter buf;
    buf.appendUInt64(time);
    buf.appendUInt64(rnd_.random64());
    buf.appendVarUInt(msg_buf.size());
    buf.append(msg_buf.data(), msg_buf.size());

    auto key = StringUtil::format("__uploadq-sessions-$0",  rnd_.hex128());
    txn->update(key.data(), key.size(), buf.data(), buf.size());
  }

  ++num_sessions;
}

} // namespace cm

