/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <stdlib.h>
#include <stdio.h>
#include "fnord-base/stdtypes.h"
#include "fnord-base/exception.h"
#include "fnord-base/test/unittest.h"
#include "fnord-msg/msg.h"
#include "src/schemas.cc"
#include "logjoin/LogJoinTarget.cc"
#include "JoinedSession.pb.h"

using namespace fnord;
using namespace cm;

UNIT_TEST(LogJoinTest);

typedef HashMap<String, HashMap<String, String>> StringMap;

LogJoinTarget mkTestTarget() {
  LogJoinTarget trgt(joinedSessionsSchema(), false);

  trgt.setNormalize([] (Language l, const String& q) { return q; });

  trgt.setGetField([] (const DocID& doc, const String& field) {
    return None<String>();
  });

  return trgt;
}

void printBuf(const Buffer& buf) {
  msg::MessageObject msg;
  msg::MessageDecoder::decode(buf, joinedSessionsSchema(), &msg);
  fnord::iputs("joined session: $0", msg::MessagePrinter::print(msg, joinedSessionsSchema()));
}

// query with click
// multiple queries
// report items in multiple batches (logenplaetze, normal, etc)
// cart // gmv matching
// field expansion // joining


TEST_CASE(LogJoinTest, SimpleQuery, [] () {
  auto t = 1432311555 * kMicrosPerSecond;
  auto trgt = mkTestTarget();

  TrackedSession sess;
  sess.insertLogline(t + 0, "q", "E1", URI::ParamList {
    { "is", "p~101~p1,p~102~p2" },
    { "qstr~de", "blah" }
  });

  auto buf = trgt.trackedSessionToJoinedSession(sess);
  auto joined = msg::decode<JoinedSession>(buf);
  EXPECT_EQ(joined.num_cart_items(), 0);
  EXPECT_EQ(joined.cart_value_eurcents(), 0);
  EXPECT_EQ(joined.num_order_items(), 0);
  EXPECT_EQ(joined.gmv_eurcents(), 0);
  EXPECT_EQ(joined.first_seen_time(), 1432311555);

  EXPECT_EQ(joined.search_queries().size(), 1);
  EXPECT_EQ(joined.search_queries().Get(0).num_result_items(), 2);
  EXPECT_EQ(joined.search_queries().Get(0).num_result_items_clicked(), 0);
  EXPECT_EQ(joined.search_queries().Get(0).num_ad_clicks(), 0);
  EXPECT_EQ(joined.search_queries().Get(0).num_cart_items(), 0);
  EXPECT_EQ(joined.search_queries().Get(0).cart_value_eurcents(), 0);
  EXPECT_EQ(joined.search_queries().Get(0).num_order_items(), 0);
  EXPECT_EQ(joined.search_queries().Get(0).gmv_eurcents(), 0);
  EXPECT_EQ(joined.search_queries().Get(0).query_string(), "blah");
  EXPECT_EQ(joined.search_queries().Get(0).query_string_normalized(), "blah");
  EXPECT_EQ(
    ProtoLanguage_Name(joined.search_queries().Get(0).language()),
    "LANGUAGE_DE");
  //DaWanda Hack
  EXPECT_EQ(joined.search_queries().Get(0).num_ad_impressions(), 2);

  EXPECT_EQ(joined.search_queries().Get(0).result_items().size(), 2);
  EXPECT_EQ(joined.search_queries().Get(0).result_items().Get(0).position(), 1);
  EXPECT_EQ(joined.search_queries().Get(0).result_items().Get(0).item_id(), "p~101");
  EXPECT_FALSE(joined.search_queries().Get(0).result_items().Get(0).clicked());

  EXPECT_EQ(joined.search_queries().Get(0).result_items().Get(1).position(), 2);
  EXPECT_EQ(joined.search_queries().Get(0).result_items().Get(1).item_id(), "p~102");
  EXPECT_FALSE(joined.search_queries().Get(0).result_items().Get(1).clicked());
});

/*

TEST_CASE(LogJoinTest, Test1, [] () {
  auto loglines_txt = FileUtil::read("src/test_loglines.txt");
  auto loglines = StringUtil::split(loglines_txt.toString(), "\n");

  for (const auto& logline : loglines) {
    if (logline.length() == 0) {
      continue;
    }

    URI::ParamList params;
    URI::parseQueryString(logline, &params);

    String time;
    String evtype;
    String evid;
    for (auto p = params.begin(); p != params.end(); ) {
      switch (p->first[0]) {

        default:
          ++p;
          continue;

        case 't':
          time = p->second;
          break;

        case 'e':
          evtype = p->second;
          break;

        case 'c':
          Vector<String> cid = StringUtil::split(p->second, "~");
          evid = cid.at(1);
          break;
      }

      p = params.erase(p);
    }

    tracked_session.insertLogline(
      DateTime((uint64_t) std::stoull(time) * 1000000),
      evtype,
      evid,
      params);
  }

  auto joined_sessions_schema = joinedSessionsSchema();
  LogJoinTarget logjoin_target(joined_sessions_schema, false);

  StringMap fields;
  fields["p~62797695"]["category1"] = "4";

  auto getField = [fields] (const DocID& docid, const String& feature) -> Option<String> {

    auto it = fields.find(docid.docid);
    if (it != fields.end()) {
      auto field = it->second;
      auto it_ = field.find(feature);
      if (it_ != field.end()) {
        return Some(String(it_->second));
      }
    }

    return None<String>();
  };

  auto normalize = [] (Language lang, const String& query) -> String {
    return query;
  };

  logjoin_target.setGetField(getField);
  logjoin_target.setNormalize(normalize);
  Buffer session = logjoin_target.trackedSessionToJoinedSession(
    tracked_session);

  JoinedSession joined_session;
  joined_session.ParseFromString(session.toString());
});

*/
