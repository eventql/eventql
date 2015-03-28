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

AutoCompleteServlet::AutoCompleteServlet(
    RefPtr<fts::Analyzer> analyzer) :
    analyzer_(analyzer) {
  cat_names_.emplace("de~e1-1",  "Mode");
  cat_names_.emplace("de~e1-2",  "Accessoires");
  cat_names_.emplace("de~e1-3",  "Taschen");
  cat_names_.emplace("de~e1-4",  "Schmuck");
  cat_names_.emplace("de~e1-5",  "Kinder");
  cat_names_.emplace("de~e1-6",  "Wohnen");
  cat_names_.emplace("de~e1-7",  "Kunst");
  cat_names_.emplace("de~e1-8",  "Handarbeitsbedarf");
  cat_names_.emplace("de~e1-9",  "Vintage");
  cat_names_.emplace("de~e1-10", "Baby");
  cat_names_.emplace("de~e1-11", "Anlass");
  cat_names_.emplace("de~e1-12", "Sale");
  cat_names_.emplace("de~e1-13", "Maenner");
  cat_names_.emplace("de~e1-14", "Schreibwaren");
}

void AutoCompleteServlet::addTermInfo(const String& term, const TermInfo& ti) {
  if (ti.score < 1000) return;
  term_info_.emplace(term, ti);
}

void AutoCompleteServlet::handleHTTPRequest(
    http::HTTPRequest* req,
    http::HTTPResponse* res) {
  URI uri(req->uri());
  ResultListType results;
  const auto& params = uri.queryParams();

  /* arguments */
  String lang_str;
  if (!URI::getParam(params, "lang", &lang_str)) {
    res->addBody("error: missing ?lang=... parameter");
    res->setStatus(http::kStatusBadRequest);
    return;
  }

  String qstr;
  if (!URI::getParam(params, "q", &qstr)) {
    res->addBody("error: missing ?q=... parameter");
    res->setStatus(http::kStatusBadRequest);
    return;
  }

  Vector<String> terms;
  Vector<String> valid_terms;
  auto lang = languageFromString(lang_str);
  analyzer_->tokenize(lang, qstr, &terms);
  if (terms.size() == 0) {
    res->addBody("error: invalid ?q=... parameter");
    res->setStatus(http::kStatusBadRequest);
    return;
  }

  for (const auto& t : terms) {
    if (term_info_.count(lang_str + "~" + t) > 0) {
      valid_terms.emplace_back(t);
    }
  }

  if (terms.size() == 1 || valid_terms.size() == 0) {
    suggestSingleTerm(lang, terms, &results);
  } else {
    suggestMultiTerm(lang, terms, valid_terms, &results);
  }

  if (results.size() == 0 && qstr.length() > 2) {
    suggestFuzzy(lang, terms, &results);
  }

#ifndef FNORD_NODEBUG
  fnord::logDebug(
      "cm.autocompleteserver",
      "suggest lang=$0 qstr=$1 terms=$2 valid_terms=$3 results=$4",
      lang_str,
      qstr,
      inspect(terms),
      inspect(valid_terms),
      results.size());
#endif

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

  for (int i = 0; i < results.size() && i < 12; ++i) {
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

void AutoCompleteServlet::suggestSingleTerm(
    Language lang,
    Vector<String> terms,
    ResultListType* results) {
  auto lang_str = fnord::languageToString(lang);
  auto prefix = lang_str + "~" + terms.back();
  terms.pop_back();
  String qstr_prefix;

  if (terms.size() > 0 ) {
    qstr_prefix += StringUtil::join(terms, " ") + " ";
  }

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
          "$0$1 in $2",
          qstr_prefix,
          matches[0].first.substr(3),
          cat_names_[lang_str + "~" + c.first]);

      results->emplace_back(label, c.second, "");
    }
  }

  for (int m = 0; m < matches.size(); ++m) {
    auto score = matches[m].second;

    if ((score / best_match) < 0.1) {
      break;
    }

    results->emplace_back(qstr_prefix + matches[m].first.substr(3), score, "");
  }

  if (matches.size() > 0) {
    const auto& best_match_ti = term_info_.find(matches[0].first);

    for (const auto& r : best_match_ti->second.related_terms) {
      auto label = StringUtil::format(
          "$0$1 $2",
          qstr_prefix,
          matches[0].first.substr(3),
          r.first);

      results->emplace_back(label, r.second, "");
    }
  }
}

void AutoCompleteServlet::suggestMultiTerm(
    Language lang,
    Vector<String> terms,
    const Vector<String>& valid_terms,
    ResultListType* results) {
  Set<String> search_terms;
  auto lang_str = fnord::languageToString(lang);
  auto last_term = terms.back();
  terms.pop_back();
  String qstr_prefix;

  if (terms.size() > 0 ) {
    qstr_prefix += StringUtil::join(terms, " ") + " ";
  }

  HashMap<String, double> topcats_h;
  double best_topcat = 0;
  HashMap<String, double> matches_h;
  double best_match = 0;
  for (const auto& vt : valid_terms) {
    const auto& vtinfo = term_info_.find(lang_str + "~" + vt)->second;
    search_terms.emplace(vt);

    for (const auto& topcat : vtinfo.top_categories) {
      topcats_h[topcat.first] += topcat.second;
      topcats_h[topcat.first] *= 2;

      if (topcats_h[topcat.first] > best_topcat) {
        best_topcat = topcats_h[topcat.first];
      }
    }

    for (const auto& related : vtinfo.related_terms) {
      if (!StringUtil::beginsWith(related.first, last_term)) {
        continue;
      }

      matches_h[related.first] += related.second;

      if (matches_h[related.first] > best_match) {
        best_match = matches_h[related.first];
      }
    }
  }

  Vector<Pair<String, double>> topcats;
  for (const auto& m : topcats_h) {
    topcats.emplace_back(m);
  }

  std::sort(topcats.begin(), topcats.end(), [] (
      const Pair<String, double>& a,
      const Pair<String, double>& b) {
    return b.second < a.second;
  });

  for (int m = 0; m < topcats.size() && m < 3; ++m) {
    auto score = topcats[m].second;

    if ((score / best_topcat) < 0.2) {
      break;
    }

    auto label = StringUtil::format(
        "$0 in $1",
        StringUtil::join(valid_terms, " "),
        cat_names_[lang_str + "~" + topcats[m].first]);

    results->emplace_back(label, score, "");
  }

  Vector<Pair<String, double>> matches;
  for (const auto& m : matches_h) {
    matches.emplace_back(m);
  }

  std::sort(matches.begin(), matches.end(), [] (
      const Pair<String, double>& a,
      const Pair<String, double>& b) {
    return b.second < a.second;
  });

  if (matches.size() > 0) {
    search_terms.emplace(matches[0].first);
  }

  for (int m = 0; m < matches.size(); ++m) {
    auto score = matches[m].second;

    if ((score / best_match) < 0.1) {
      break;
    }

    results->emplace_back(qstr_prefix + matches[m].first, score, "");
  }

  matches_h.clear();
  matches.clear();
  best_match = 0;

  for (const auto& vt : search_terms) {
    const auto& vtinfo = term_info_.find(lang_str + "~" + vt)->second;
    for (const auto& related : vtinfo.related_terms) {
      matches_h[related.first] += related.second;

      if (matches_h[related.first] > best_match) {
        best_match = matches_h[related.first];
      }
    }
  }

  for (const auto& m : matches_h) {
    matches.emplace_back(m);
  }

  std::sort(matches.begin(), matches.end(), [] (
      const Pair<String, double>& a,
      const Pair<String, double>& b) {
    return b.second < a.second;
  });

  for (int m = 0; m < matches.size(); ++m) {
    auto score = matches[m].second;

    if ((score / best_match) < 0.1) {
      break;
    }

    results->emplace_back(qstr_prefix + matches[m].first, score, "");
  }
}

void AutoCompleteServlet::suggestFuzzy(
    Language lang,
    Vector<String> terms,
    ResultListType* results) {
  results->emplace_back("here be dragons: fuzzy suggestion", 1.0, "");
}


}
