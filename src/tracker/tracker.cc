/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "tracker.h"
#include <fnord/base/inspect.h>
#include <fnord/net/http/cookies.h>
#include "fnord/base/random.h"

namespace cm {

const char Tracker::kUIDCookieKey[] = "_u";
const int Tracker::kUIDCookieLifetimeDays = 365 * 5;

Tracker::Tracker() {}

bool Tracker::handleHTTPRequest(
    fnord::http::HTTPRequest* request,
    fnord::http::HTTPResponse* response) {

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

  fnord::iputs("uid: $0", uid);

  response->setStatus(fnord::http::kStatusOK);
  return true;
}

} // namespace cm
