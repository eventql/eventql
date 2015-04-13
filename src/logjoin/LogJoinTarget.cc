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
#include <fnord-fts/fts.h>
#include <fnord-fts/fts_common.h>
#include "logjoin/LogJoinTarget.h"
#include "common.h"

using namespace fnord;

namespace cm {

LogJoinTarget::LogJoinTarget(
    const msg::MessageSchema& joined_sessions_schema,
    fts::Analyzer* analyzer) :
    joined_sessions_schema_(joined_sessions_schema),
    analyzer_(analyzer),
    num_sessions(0),
    num_queries(0),
    num_item_visits(0) {}

void LogJoinTarget::onSession(
    mdb::MDBTransaction* txn,
    const TrackedSession& session) {
  const auto& schema = joined_sessions_schema_;
  msg::MessageObject obj;

  for (const auto& tq : session.flushed_queries) {
    auto q = trackedQueryToJoinedQuery(session, tq);
    auto& qry_obj = obj.addChild(schema.id("queries"));

    /* queries.time */
    qry_obj.addChild(
        schema.id("queries.time"),
        (uint32_t) (q.time.unixMicros() / kMicrosPerSecond));

    /* queries.language */
    auto lang = cm::extractLanguage(q.attrs);
    qry_obj.addChild(schema.id("queries.language"), (uint32_t) lang);

    /* queries.query_string */
    auto qstr = cm::extractQueryString(q.attrs);
    if (!qstr.isEmpty()) {
      auto qstr_norm = analyzer_->normalize(lang, qstr.get());
      qry_obj.addChild(schema.id("queries.query_string"), qstr.get());
      qry_obj.addChild(schema.id("queries.query_string_normalized"), qstr_norm);
    }

    /* queries.num_item_clicks, queries.num_items */
    uint32_t nitems = 0;
    uint32_t nclicks = 0;
    uint32_t nads = 0;
    uint32_t nadclicks = 0;
    for (const auto& i : q.items) {
      // DAWANDA HACK
      if (i.position >= 1 && i.position <= 4) {
        ++nads;
        nadclicks += i.clicked;
      }
      // EOF DAWANDA HACK

      ++nitems;
      nclicks += i.clicked;
    }

    qry_obj.addChild(schema.id("queries.num_items"), nitems);
    qry_obj.addChild(schema.id("queries.num_items_clicked"), nclicks);
    qry_obj.addChild(schema.id("queries.num_ad_impressions"), nads);
    qry_obj.addChild(schema.id("queries.num_ad_clicks"), nadclicks);

    /* queries.page */
    auto pg_str = cm::extractAttr(q.attrs, "pg");
    if (!pg_str.isEmpty()) {
      uint32_t pg = std::stoul(pg_str.get());
      qry_obj.addChild(schema.id("queries.page"), pg);
    }

    /* queries.ab_test_group */
    auto abgrp = cm::extractABTestGroup(q.attrs);
    if (!abgrp.isEmpty()) {
      qry_obj.addChild(schema.id("queries.ab_test_group"), abgrp.get());
    }

    /* queries.category1 */
    auto qcat1 = cm::extractAttr(q.attrs, "q_cat1");
    if (!qcat1.isEmpty()) {
      uint32_t c = std::stoul(qcat1.get());
      qry_obj.addChild(schema.id("queries.category1"), c);
    }

    /* queries.category1 */
    auto qcat2 = cm::extractAttr(q.attrs, "q_cat2");
    if (!qcat2.isEmpty()) {
      uint32_t c = std::stoul(qcat2.get());
      qry_obj.addChild(schema.id("queries.category2"), c);
    }

    /* queries.category1 */
    auto qcat3 = cm::extractAttr(q.attrs, "q_cat3");
    if (!qcat3.isEmpty()) {
      uint32_t c = std::stoul(qcat3.get());
      qry_obj.addChild(schema.id("queries.category3"), c);
    }

    /* queries.device_type */
    qry_obj.addChild(
        schema.id("queries.device_type"),
        (uint32_t) extractDeviceType(q.attrs));

    /* queries.page_type */
    qry_obj.addChild(
        schema.id("queries.page_type"),
        (uint32_t) extractPageType(q.attrs));

    for (const auto& item : q.items) {
      auto& item_obj = qry_obj.addChild(schema.id("queries.items"));
      item_obj.addChild(
          schema.id("queries.items.position"),
          (uint32_t) item.position);
      if (item.clicked) {
        item_obj.addChild(schema.id("queries.items.clicked"), msg::TRUE);
      } else {
        item_obj.addChild(schema.id("queries.items.clicked"), msg::FALSE);
      }
    }
  }

  Buffer msg_buf;
  msg::MessageEncoder::encode(obj, joined_sessions_schema_, &msg_buf);
  auto key = StringUtil::format("__uploadq-sessions-$0",  rnd_.hex128());
  txn->update(key, msg_buf);
  ++num_sessions;
}

void LogJoinTarget::onQuery(
    mdb::MDBTransaction* txn,
    const TrackedSession& session,
    const TrackedQuery& query) {
  //fnord::iputs("on query... $0", query.items.size());
  ++num_queries;
}

void LogJoinTarget::onItemVisit(
    mdb::MDBTransaction* txn,
    const TrackedSession& session,
    const TrackedItemVisit& item_visit) {

}

void LogJoinTarget::onItemVisit(
    mdb::MDBTransaction* txn,
    const TrackedSession& session,
    const TrackedItemVisit& item_visit,
    const TrackedQuery& query) {
  ++num_item_visits;
}

JoinedQuery LogJoinTarget::trackedQueryToJoinedQuery(
    const TrackedSession& session,
    const TrackedQuery& query) const {
  JoinedQuery jq;
  jq.customer = session.customer_key;
  jq.uid = session.uid;
  jq.time = query.time;
  jq.attrs = query.attrs;
  for (const auto& a : session.attrs) {
    jq.attrs.emplace_back("s!" + a);
  }

  for (const auto& item : query.items) {
    JoinedQueryItem jqi;
    jqi.item = item.item;
    jqi.clicked = item.clicked;
    jqi.variant = item.variant;
    jqi.position = item.position;
    jq.items.emplace_back(jqi);
  }

  return jq;
}

/*
void LogJoin::onQuery(
    const TrackedSession& session,
    const TrackedQuery& query) {
  stat_joined_queries_.incr(1);
  auto jq_json = json::toJSONString(jq);
  auto feeds = getFeedsForCustomer(jq.customer);

  if (dry_run_) {
#ifndef NDEBUG
    fnord::logTrace(
        "cm.logjoin",
        "[dry-run] would write joined query: $0",
        jq_json);
#endif
  } else {
    feeds->joined_queries_feed_writer->appendEntry(jq_json);
  }
}

void LogJoin::onItemVisit(
    const TrackedSession& session,
    const TrackedItemVisit& item_visit) {
  stat_joined_item_visits_.incr(1);

  JoinedItemVisit jiv;
  jiv.customer = session.customer_key;
  jiv.uid = session.uid;
  jiv.time = item_visit.time;
  jiv.item = item_visit.item;
  jiv.attrs = item_visit.attrs;

  for (const auto& a : session.attrs) {
    jiv.attrs.emplace_back("s!" + a);
  }

  auto jiv_json = json::toJSONString(jiv);
  auto feeds = getFeedsForCustomer(jiv.customer);

  if (dry_run_) {
#ifndef NDEBUG
    fnord::logTrace(
        "cm.logjoin",
        "[dry-run] would write joined item visit: $0",
        jiv_json);
#endif
  } else {
    feeds->joined_item_visits_feed_writer->appendEntry(jiv_json);
  }
}

void LogJoin::onItemVisit(
    const TrackedSession& session,
    const TrackedItemVisit& item_visit,
    const TrackedQuery& query) {
  stat_joined_item_visits_.incr(1);

  JoinedItemVisit jiv;
  jiv.customer = session.customer_key;
  jiv.uid = session.uid;
  jiv.time = item_visit.time;
  jiv.item = item_visit.item;
  jiv.attrs = item_visit.attrs;

  for (const auto& a : session.attrs) {
    jiv.attrs.emplace_back("s!" + a);
  }

  for (const auto& a : query.attrs) {
    jiv.attrs.emplace_back("q!" + a);
  }

  auto jiv_json = json::toJSONString(jiv);
  auto feeds = getFeedsForCustomer(jiv.customer);

  if (dry_run_) {
#ifndef NDEBUG
    fnord::logTrace(
        "cm.logjoin",
        "[dry-run] would write joined item visit: $0",
        jiv_json);
#endif
  } else {
    feeds->joined_item_visits_feed_writer->appendEntry(jiv_json);
  }
}
*/

} // namespace cm

