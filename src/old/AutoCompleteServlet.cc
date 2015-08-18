/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "AutoCompleteServlet.h"
#include "zbase/CTRCounter.h"
#include "stx/Language.h"
#include "stx/logging.h"
#include "stx/wallclock.h"
#include "stx/io/fileutil.h"

using namespace stx;

namespace zbase {

AutoCompleteServlet::AutoCompleteServlet(
    ModelCache* models) :
    models_(models) {
  exportStat(
      "/cm-autocompleteserver/global/requests_total",
      &stat_requests_total_,
      stx::stats::ExportMode::EXPORT_DELTA);
}

void AutoCompleteServlet::handleHTTPRequest(
    http::HTTPRequest* req,
    http::HTTPResponse* res) {
  stat_requests_total_.incr(1);

  res->addHeader("Access-Control-Allow-Origin", "*");

  URI uri(req->uri());
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

  if (qstr.size() == 0) {
    res->addBody("error: invalid ?q=... parameter");
    res->setStatus(http::kStatusBadRequest);
    return;
  }

  auto lang = languageFromString(lang_str);
  auto model = AutoCompleteModel::fromCache("dawanda", models_);
  AutoCompleteModel::ResultListType results;
  model->suggest(lang, qstr, &results);

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
    generateURL(&results[i]);

    if (i > 0) {
      json.addComma();
    }
    json.beginObject();
    json.addObjectEntry("text");
    json.addString(results[i].text);
    json.addComma();
    json.addObjectEntry("score");
    json.addFloat(results[i].score);
    json.addComma();
    json.addObjectEntry("url");
    json.addString(results[i].url);
    json.addComma();
    json.addObjectEntry("attrs");
    toJSON(results[i].attrs, &json);
    json.endObject();
  }

  json.endArray();
  json.endObject();
}

void AutoCompleteServlet::generateURL(AutoCompleteResult* result) {
  result->url = "/search?q=" + URI::urlEncode(result->attrs["query_string"]);

  auto cat_i = result->attrs.find("category_id");
  if (cat_i != result->attrs.end()) {
    String cat = cat_i->second;
    cat.erase(cat.begin(), cat.begin() + cat.find("-") + 1);
    result->url += "&category_id=" + cat;
  }
}

}
