/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "frontend/LookupServlet.h"
#include "sellerstats/SellerStatsLookup.h"

using namespace stx;

namespace zbase {

LookupServlet::LookupServlet(const String& cmdata) : cmdata_(cmdata) {}

void LookupServlet::handleHTTPRequest(
    stx::http::HTTPRequest* req,
    stx::http::HTTPResponse* res) {
  URI uri(req->uri());
  URI::ParamList params = uri.queryParams();

  res->addHeader("Access-Control-Allow-Origin", "*");

  std::string lookup_type;
  if (!stx::URI::getParam(params, "lookup", &lookup_type)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing parameter ?lookup=...");
    return;
  }

  /* sellerstats lookup */
  if (lookup_type == "sellerstats") {
    lookupSellerStats(req, res);
    return;
  }

  res->setStatus(http::kStatusBadRequest);
  res->addBody("unknown lookup type: " + lookup_type);
}

void LookupServlet::lookupSellerStats(
    http::HTTPRequest* req,
    http::HTTPResponse* res) {
  URI uri(req->uri());
  URI::ParamList params = uri.queryParams();

  std::string shopid;
  if (!stx::URI::getParam(params, "shopid", &shopid)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing parameter ?shopid=...");
    return;
  }

  std::string customer;
  if (!stx::URI::getParam(params, "customer", &customer)) {
    res->setStatus(http::kStatusBadRequest);
    res->addBody("missing parameter ?customer=...");
    return;
  }

  try {
    auto lookup = SellerStatsLookup::lookup(shopid, cmdata_, customer);
    res->setStatus(http::kStatusOK);
    res->addBody(lookup);
  } catch (const Exception& e) {
    res->setStatus(http::kStatusInternalServerError);
    res->addBody("lookup failed: " + e.getMessage());
    return;
  }
}



}
