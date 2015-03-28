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
  //if (ti.score < 1000) return;
  term_info_[term] = ti;
}

void AutoCompleteServlet::handleHTTPRequest(
    http::HTTPRequest* req,
    http::HTTPResponse* res) {
  URI uri(req->uri());

  const auto& params = uri.queryParams();

  /* arguments */
  String qstr;
  if (!URI::getParam(params, "q", &qstr)) {
    res->addBody("error: missing ?q=... parameter");
    res->setStatus(http::kStatusBadRequest);
    return;
  }

  /* write response */
  res->setStatus(http::kStatusOK);
  res->addHeader("Content-Type", "application/json; charset=utf-8");
  json::JSONOutputStream json(res->getBodyOutputStream());

  json.beginObject();
  json.addObjectEntry("query");
  json.addString(qstr);
  json.endObject();
}

}
