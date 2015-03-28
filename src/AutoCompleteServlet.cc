/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "AutoCompleteServlet.h"
#include "CTRCounter.h"
#include "fnord-base/Language.h"
#include "fnord-base/wallclock.h"
#include "fnord-base/io/fileutil.h"

using namespace fnord;

namespace cm {

AutoCompleteServlet::AutoCompleteServlet() {}

void AutoCompleteServlet::addTermInfo(const String& term, const TermInfo& ti) {
  if (ti.score < 1000) return;
  term_info_.emplace(term, ti);
}

void AutoCompleteServlet::handleHTTPRequest(
    http::HTTPRequest* req,
    http::HTTPResponse* res) {
  URI uri(req->uri());
  Vector<Tuple<String, uint64_t, String>> results;
  const auto& params = uri.queryParams();

  /* arguments */
  String lang_str;
  if (!URI::getParam(params, "lang", &lang_str)) {
    res->addBody("error: missing ?lang=... parameter");
    res->setStatus(http::kStatusBadRequest);
    return;
  }

  auto lang = languageFromString(lang_str);

  String qstr;
  if (!URI::getParam(params, "q", &qstr)) {
    res->addBody("error: missing ?q=... parameter");
    res->setStatus(http::kStatusBadRequest);
    return;
  }

  auto prefix = languageToString(lang) + "~" + qstr;

  Vector<Pair<String, double>> matches;
  double best_match = 0;
  for (auto iter = term_info_.lower_bound(prefix);
      iter != term_info_.end() && StringUtil::beginsWith(iter->first, prefix);
      ++iter) {
    matches.emplace_back(iter->first, iter->second.score);
    if (iter->second.score > best_match) {
      best_match = iter->second.score;
    }
  }

  std::sort(matches.begin(), matches.end(), [] (
      const Pair<String, double>& a,
      const Pair<String, double>& b) {
    return b.second < a.second;
  });

  if (matches.size() > 0) {
    const auto& best_match_ti = term_info_.find(matches[0].first);
    for (const auto& c : best_match_ti->second.top_categories) {
      if ((c.second / best_match_ti->second.top_categories[0].second) < 0.2) {
        break;
      }

      auto label = StringUtil::format(
          "$0 in $1",
          matches[0].first,
          c.first);

      results.emplace_back(label, c.second, "");
    }
  }

  for (int i = 0; i < matches.size(); ++i) {
    auto score = matches[i].second;

    if ((score / best_match) < 0.1) {
      break;
    }

    results.emplace_back(matches[i].first, score, "");
  }

  /* write response */
  res->setStatus(http::kStatusOK);
  res->addHeader("Content-Type", "application/json; charset=utf-8");
  json::JSONOutputStream json(res->getBodyOutputStream());

  json.beginObject();
  json.addObjectEntry("query");
  json.addString(qstr);
  json.addComma();
  json.addObjectEntry("suggestions");
  json.beginArray();

  for (int i = 0; i < results.size(); ++i) {
    if (i > 0) {
      json.addComma();
    }
    json.beginObject();
    json.addObjectEntry("text");
    json.addString(std::get<0>(results[i]));
    json.addComma();
    json.addObjectEntry("score");
    json.addFloat(std::get<1>(results[i]));
    json.addComma();
    json.addObjectEntry("url");
    json.addString(std::get<2>(results[i]));
    json.endObject();
  }

  json.endArray();
  json.endObject();
}

}
