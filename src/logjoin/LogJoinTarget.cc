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

void LogJoinTarget::onSession(
    mdb::MDBTransaction* txn,
    const TrackedSession& session) {
  fnord::iputs("on session... $0", session.flushed_queries.size());
  const auto& schema = joined_sessions_schema_;
  msg::MessageObject obj;

  for (const auto& tq : session.flushed_queries) {
    auto q = trackedQueryToJoinedQuery(tq);
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
      item_obj.addChild(schema.id("queries.items.clicked"), item.clicked);
    }
  }

}

void LogJoinTarget::onQuery(
    mdb::MDBTransaction* txn,
    const TrackedSession& session,
    const TrackedQuery& query) {
  //fnord::iputs("on query... $0", query.items.size());
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

}

} // namespace cm

