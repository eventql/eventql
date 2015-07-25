/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "stx/protobuf/MessageBuilder.h"
#include "stx/protobuf/MessageObject.h"
#include "stx/protobuf/MessageEncoder.h"
#include "stx/protobuf/MessageDecoder.h"
#include "stx/protobuf/MessagePrinter.h"
#include "stx/protobuf/msg.h"
#include "logjoin/LogJoinTarget.h"
#include "common.h"

using namespace stx;

namespace cm {

LogJoinTarget::LogJoinTarget(
    msg::MessageSchemaRepository* schemas,
    bool dry_run) :
    num_sessions(0),
    schemas_(schemas),
    dry_run_(dry_run),
    cconv_(currencyConversionTable()),
    tpool_(
        4,
        mkScoped(new CatchAndLogExceptionHandler("logjoind")),
        100,
        true) {}

void LogJoinTarget::start() {
  tpool_.start();
}

void LogJoinTarget::stop() {
  tpool_.stop();
}

void LogJoinTarget::enqueueSession(const TrackedSession& session) {
  // FIXPAUL: write to some form of persisten queue
  tpool_.run(
      std::bind(
          &LogJoinTarget::processSession,
          this,
          session));
}

void LogJoinTarget::processSession(TrackedSession session) {
  stx::iputs("process session...", 1);
  session.debugPrint();
}

void LogJoinTarget::setNormalize(
    Function<stx::String (Language lang, const stx::String& query)> normalizeCb) {
  normalize_ = normalizeCb;
}

void LogJoinTarget::setGetField(
    Function<Option<String> (const DocID& docid, const String& feature)> getFieldCb) {
  get_field_ = getFieldCb;
}

Buffer LogJoinTarget::joinSession(TrackedSession& session) {
  if (!get_field_) {
    RAISE(kRuntimeError, "getField has not been initialized");
  }

  if (!normalize_) {
    RAISE(kRuntimeError, "normalize has not been initialized");
  }

  session.joinEvents(cconv_);
  auto session_schema = schemas_->getSchema("cm.JoinedSession");
  auto searchq_schema = schemas_->getSchema("cm.JoinedSearchQuery");
  auto searchq_item_schema = schemas_->getSchema("cm.JoinedSearchQueryResultItem");
  auto cart_item_schema = schemas_->getSchema("cm.JoinedCartItem");
  auto item_visit_schema = schemas_->getSchema("cm.JoinedItemVisit");
  msg::MessageObject obj;

  for (const auto& ci : session.cart_items) {
    auto& ci_obj = obj.addChild(session_schema->fieldId("cart_items"));

    ci_obj.addChild(
        cart_item_schema->fieldId("time"),
        (uint32_t) (ci.time.unixMicros() / kMicrosPerSecond));

    ci_obj.addChild(
        cart_item_schema->fieldId("item_id"),
        ci.item.docID().docid);

    ci_obj.addChild(
        cart_item_schema->fieldId("quantity"),
        ci.quantity);

    ci_obj.addChild(
        cart_item_schema->fieldId("price_cents"),
        ci.price_cents);

    ci_obj.addChild(
        cart_item_schema->fieldId("currency"),
        (uint32_t) currencyFromString(ci.currency));

    ci_obj.addChild(
        cart_item_schema->fieldId("checkout_step"),
        ci.checkout_step);

    // FIXPAUL use getFields...
    auto docid = ci.item.docID();
    auto shopid = get_field_(docid, "shop_id");
    if (shopid.isEmpty()) {
      //stx::logWarning(
      //    "cm.logjoin",
      //    "item not found in featureindex: $0",
      //    docid.docid);
    } else {
      ci_obj.addChild(
          cart_item_schema->fieldId("shop_id"),
          (uint32_t) std::stoull(shopid.get()));
    }

    auto category1 = get_field_(docid, "category1");
    if (!category1.isEmpty()) {
      ci_obj.addChild(
          cart_item_schema->fieldId("category1"),
          (uint32_t) std::stoull(category1.get()));
    }

    auto category2 = get_field_(docid, "category2");
    if (!category2.isEmpty()) {
      ci_obj.addChild(
          cart_item_schema->fieldId("category2"),
          (uint32_t) std::stoull(category2.get()));
    }

    auto category3 = get_field_(docid, "category3");
    if (!category3.isEmpty()) {
      ci_obj.addChild(
          cart_item_schema->fieldId("category3"),
          (uint32_t) std::stoull(category3.get()));
    }
  }

  uint32_t sess_abgrp = 0;
  for (const auto& q : session.queries) {
    auto& qry_obj = obj.addChild(session_schema->fieldId("search_queries"));

    /* queries.time */
    qry_obj.addChild(
        searchq_schema->fieldId("time"),
        (uint32_t) (q.time.unixMicros() / kMicrosPerSecond));

    /* queries.language */
    auto lang = cm::extractLanguage(q.attrs);
    qry_obj.addChild(searchq_schema->fieldId("language"), (uint32_t) lang);

    /* queries.query_string */
    auto qstr = cm::extractQueryString(q.attrs);
    if (!qstr.isEmpty()) {
      auto qstr_norm = normalize_(lang, qstr.get());
      qry_obj.addChild(searchq_schema->fieldId("query_string"), qstr.get());
      qry_obj.addChild(searchq_schema->fieldId("query_string_normalized"), qstr_norm);
    }

    /* queries.shopid */
    auto slrid = cm::extractAttr(q.attrs, "slrid");
    if (!slrid.isEmpty()) {
      uint32_t sid = std::stoul(slrid.get());
      qry_obj.addChild(searchq_schema->fieldId("shop_id"), sid);
    }

    qry_obj.addChild(searchq_schema->fieldId("num_result_items"), q.nitems);
    qry_obj.addChild(searchq_schema->fieldId("num_result_items_clicked"), q.nclicks);
    qry_obj.addChild(searchq_schema->fieldId("num_ad_impressions"), q.nads);
    qry_obj.addChild(searchq_schema->fieldId("num_ad_clicks"), q.nadclicks);
    qry_obj.addChild(searchq_schema->fieldId("num_cart_items"), q.num_cart_items);
    qry_obj.addChild(searchq_schema->fieldId("cart_value_eurcents"), q.cart_value_eurcents);
    qry_obj.addChild(searchq_schema->fieldId("num_order_items"), q.num_order_items);
    qry_obj.addChild(searchq_schema->fieldId("gmv_eurcents"), q.gmv_eurcents);

    /* queries.page */
    auto pg_str = cm::extractAttr(q.attrs, "pg");
    if (!pg_str.isEmpty()) {
      uint32_t pg = std::stoul(pg_str.get());
      qry_obj.addChild(searchq_schema->fieldId("page"), pg);
    }

    /* queries.ab_test_group */
    auto abgrp = cm::extractABTestGroup(q.attrs);
    if (!abgrp.isEmpty()) {
      sess_abgrp = abgrp.get();
      qry_obj.addChild(searchq_schema->fieldId("ab_test_group"), abgrp.get());
    }

    /* queries.experiments */
    auto qexps = q.joinedExperiments();
    if (qexps.size() > 0) {
      qry_obj.addChild(searchq_schema->fieldId("experiments"), qexps);
    }

    /* queries.category1 */
    auto qcat1 = cm::extractAttr(q.attrs, "q_cat1");
    if (!qcat1.isEmpty()) {
      uint32_t c = std::stoul(qcat1.get());
      qry_obj.addChild(searchq_schema->fieldId("category1"), c);
    }

    /* queries.category1 */
    auto qcat2 = cm::extractAttr(q.attrs, "q_cat2");
    if (!qcat2.isEmpty()) {
      uint32_t c = std::stoul(qcat2.get());
      qry_obj.addChild(searchq_schema->fieldId("category2"), c);
    }

    /* queries.category1 */
    auto qcat3 = cm::extractAttr(q.attrs, "q_cat3");
    if (!qcat3.isEmpty()) {
      uint32_t c = std::stoul(qcat3.get());
      qry_obj.addChild(searchq_schema->fieldId("category3"), c);
    }

    /* queries.device_type */
    qry_obj.addChild(
        searchq_schema->fieldId("device_type"),
        (uint32_t) extractDeviceType(q.attrs));

    /* queries.page_type */
    auto page_type = extractPageType(q.attrs);
    qry_obj.addChild(
        searchq_schema->fieldId("page_type"),
        (uint32_t) page_type);

    /* queries.query_type */
    String query_type = pageTypeToString(page_type);
    auto qtype_attr = cm::extractAttr(q.attrs, "qt");
    if (!qtype_attr.isEmpty()) {
      query_type = qtype_attr.get();
    }
    qry_obj.addChild(
        searchq_schema->fieldId("query_type"),
        query_type);

    for (const auto& item : q.items) {
      auto& item_obj = qry_obj.addChild(
          searchq_schema->fieldId("result_items"));

      item_obj.addChild(
          searchq_item_schema->fieldId("position"),
          (uint32_t) item.position);

      item_obj.addChild(
          searchq_item_schema->fieldId("item_id"),
          item.item.docID().docid);

      if (item.clicked) {
        item_obj.addChild(
            searchq_item_schema->fieldId("clicked"),
            msg::TRUE);
      } else {
        item_obj.addChild(
            searchq_item_schema->fieldId("clicked"),
            msg::FALSE);
      }

      if (item.seen) {
        item_obj.addChild(
            searchq_item_schema->fieldId("seen"),
            msg::TRUE);
      } else {
        item_obj.addChild(
            searchq_item_schema->fieldId("seen"),
            msg::FALSE);
      }

      auto docid = item.item.docID();

      auto shopid = get_field_(docid, "shop_id");
      if (shopid.isEmpty()) {
        //stx::logWarning(
        //    "cm.logjoin",
        //    "item not found in featureindex: $0",
        //    docid.docid);
      } else {
        item_obj.addChild(
            searchq_item_schema->fieldId("shop_id"),
            (uint32_t) std::stoull(shopid.get()));
      }

      auto category1 = get_field_(docid, "category1");
      if (!category1.isEmpty()) {
        item_obj.addChild(
            searchq_item_schema->fieldId("category1"),
            (uint32_t) std::stoull(category1.get()));
      }

      auto category2 = get_field_(docid, "category2");
      if (!category2.isEmpty()) {
        item_obj.addChild(
            searchq_item_schema->fieldId("category2"),
            (uint32_t) std::stoull(category2.get()));
      }

      auto category3 = get_field_(docid, "category3");
      if (!category3.isEmpty()) {
        item_obj.addChild(
            searchq_item_schema->fieldId("category3"),
            (uint32_t) std::stoull(category3.get()));
      }

      /* DAWANDA HACK */
      if (item.position <= 4 && slrid.isEmpty()) {
        item_obj.addChild(
            searchq_item_schema->fieldId("is_paid_result"),
            msg::TRUE);
      }

      if ((item.position > 40 && slrid.isEmpty()) ||
          StringUtil::beginsWith(query_type, "recos_")) {
        item_obj.addChild(
            searchq_item_schema->fieldId("is_recommendation"),
            msg::TRUE);
      }
      /* EOF DAWANDA HACK */
    }
  }

  for (const auto& iv : session.item_visits) {
    auto& iv_obj = obj.addChild(session_schema->fieldId("item_visits"));

    iv_obj.addChild(
        item_visit_schema->fieldId("time"),
        (uint32_t) (iv.time.unixMicros() / kMicrosPerSecond));

    iv_obj.addChild(
        item_visit_schema->fieldId("item_id"),
        iv.item.docID().docid);

    auto docid = iv.item.docID();
    auto shopid = get_field_(docid, "shop_id");
    if (shopid.isEmpty()) {
      //stx::logWarning(
      //    "cm.logjoin",
      //    "item not found in featureindex: $0",
      //    docid.docid);
    } else {
      iv_obj.addChild(
          item_visit_schema->fieldId("shop_id"),
          (uint32_t) std::stoull(shopid.get()));
    }

    auto category1 = get_field_(docid, "category1");
    if (!category1.isEmpty()) {
      iv_obj.addChild(
          item_visit_schema->fieldId("category1"),
          (uint32_t) std::stoull(category1.get()));
    }

    auto category2 = get_field_(docid, "category2");
    if (!category2.isEmpty()) {
      iv_obj.addChild(
          item_visit_schema->fieldId("category2"),
          (uint32_t) std::stoull(category2.get()));
    }

    auto category3 = get_field_(docid, "category3");
    if (!category3.isEmpty()) {
      iv_obj.addChild(
          item_visit_schema->fieldId("category3"),
          (uint32_t) std::stoull(category3.get()));
    }
  }

  if (sess_abgrp > 0) {
    obj.addChild(session_schema->fieldId("ab_test_group"), sess_abgrp);
  }

  auto exps = session.joinedExperiments();
  if (exps.size() > 0) {
    obj.addChild(session_schema->fieldId("experiments"), exps);
  }

  if (!session.referrer_url.isEmpty()) {
    obj.addChild(session_schema->fieldId("referrer_url"), session.referrer_url.get());
  }

  if (!session.referrer_campaign.isEmpty()) {
    obj.addChild(session_schema->fieldId("referrer_campaign"), session.referrer_campaign.get());
  }

  if (!session.referrer_name.isEmpty()) {
    obj.addChild(session_schema->fieldId("referrer_name"), session.referrer_name.get());
  }

  if (!session.customer_session_id.isEmpty()) {
    obj.addChild(session_schema->fieldId("customer_session_id"), session.customer_session_id.get());
  }

  obj.addChild(session_schema->fieldId("num_cart_items"), session.num_cart_items);
  obj.addChild(session_schema->fieldId("cart_value_eurcents"), session.cart_value_eurcents);
  obj.addChild(session_schema->fieldId("num_order_items"), session.num_order_items);
  obj.addChild(session_schema->fieldId("gmv_eurcents"), session.gmv_eurcents);
  obj.addChild(session_schema->fieldId("customer"), session.customer_key);

  Buffer msg_buf;
  auto first_seen = session.firstSeenTime();
  auto last_seen = session.lastSeenTime();
  if (first_seen.isEmpty() || last_seen.isEmpty()) {
    RAISE(kRuntimeError, "session: time isn't set");
  }

  obj.addChild(
      session_schema->fieldId("first_seen_time"),
      first_seen.get().unixMicros() / kMicrosPerSecond);

  obj.addChild(
      session_schema->fieldId("last_seen_time"),
      last_seen.get().unixMicros() / kMicrosPerSecond);

  msg::MessageEncoder::encode(obj, *session_schema, &msg_buf);
  return msg_buf;
}


} // namespace cm

