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
#include "src/schemas.cc"
#include "logjoin/LogJoinTarget.cc"
#include "JoinedSession.pb.h"

using namespace fnord;
using namespace cm;

UNIT_TEST(LogJoinTest);


typedef HashMap<String, HashMap<String, String>> StringMap;


TEST_CASE(LogJoinTest, Test1, [] () {
  TrackedSession tracked_session;
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
    /*
    if (fields.find(docid.docid) != fields.end() &&
        fields[docid.docid].find(feature) != fields[docid.docid].end()) {

    }
    */
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
