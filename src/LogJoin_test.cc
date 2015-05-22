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
#include "fnord-base/exception.h"
#include "fnord-base/test/unittest.h"
#include "src/schemas.cc"
#include "logjoin/LogJoinTarget.cc"

using namespace fnord;
using namespace cm;

UNIT_TEST(LogJoinTest);

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
      DateTime((uint64_t) std::stoull(time)),
      evtype,
      evid,
      params);
  }

  CurrencyConverter::ConversionTable tbl;
  tbl.emplace_back(Currency::PLN, Currency::EUR, 0.25);
  CurrencyConverter cconv(tbl);
  tracked_session.joinEvents(cconv);

  auto joined_sessions_schema = joinedSessionsSchema();
  LogJoinTarget logjoin_target(joined_sessions_schema, false);

  auto getField = [] (const DocID& docid, const String& feature) -> Option<String> {
    return None<String>();
  };

  auto normalize = [] (Language lang, const String& query) -> String {
    return query;
  };

  logjoin_target.setGetField(getField);
  logjoin_target.setNormalize(normalize);
  Buffer joined_session = logjoin_target.trackedSessionToJoinedSession(
    tracked_session);
});
