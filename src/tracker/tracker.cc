/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "tracker/tracker.h"
#include <fnord/base/exception.h>
#include <fnord/base/inspect.h>
#include <fnord/net/http/cookies.h>
#include "fnord/base/random.h"
#include "customernamespace.h"

namespace cm {

const char Tracker::kUIDCookieKey[] = "_u";
const int Tracker::kUIDCookieLifetimeDays = 365 * 5;

Tracker::Tracker() {}

bool Tracker::handleHTTPRequest(
    fnord::http::HTTPRequest* request,
    fnord::http::HTTPResponse* response) {
  /* find namespace */
  CustomerNamespace* ns = nullptr;
  auto ns_iter = vhosts_.find(request->getHeader("host"));
  if (ns_iter == vhosts_.end()) {
    return false;
  } else {
    ns = ns_iter->second;
  }

  /* get or assign uid */
  std::string uid;
  auto cookies = request->cookies();
  if (!fnord::http::Cookies::getCookie(cookies, kUIDCookieKey, &uid)) {
    uid = rnd_.hex128();
    response->addCookie(
        kUIDCookieKey,
        uid,
        fnord::DateTime::daysFromNow(kUIDCookieLifetimeDays));
  }

  fnord::URI uri(request->getUrl());
  fnord::iputs("uid: $0, namespace $1, req $2", uid, ns, uri.path());

  if (uri.path() == "/report.js") {
    response->setStatus(fnord::http::kStatusOK);
    response->addHeader("Content-Type", "application/javascript");
    response->addBody(ns->trackingJS());
    return true;
  }

  response->setStatus(fnord::http::kStatusNotFound);
  return true;
}

void Tracker::addCustomer(CustomerNamespace* customer) {
  for (const auto& vhost : customer->vhosts()) {
    if (vhosts_.count(vhost) != 0) {
      RAISEF(kRuntimeError, "hostname is already registered: $0", vhost);
    }

    vhosts_[vhost] = customer;
  }
}

} // namespace cm
