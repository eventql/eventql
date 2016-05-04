/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <eventql/HTTPAuth.h>
#include "eventql/util/http/cookies.h"

namespace zbase {

const char HTTPAuth::kSessionCookieKey[] = "__dxa_session";
const uint64_t HTTPAuth::kSessionLifetimeMicros = 365 * kMicrosPerDay;

Option<AnalyticsSession> HTTPAuth::authenticateRequest(
    const http::HTTPRequest& request,
    AnalyticsAuth* auth) {
  try {
    String cookie;

    if (request.hasHeader("Authorization")) {
      static const String hdrprefix = "Token ";
      auto hdrval = request.getHeader("Authorization");
      if (StringUtil::beginsWith(hdrval, hdrprefix)) {
        cookie = URI::urlDecode(hdrval.substr(hdrprefix.size()));
      }
    }

    if (cookie.empty()) {
      const auto& cookies = request.cookies();
      http::Cookies::getCookie(cookies, kSessionCookieKey, &cookie);
    }

    if (cookie.empty()) {
      return None<AnalyticsSession>();
    }

    return auth->decodeAuthToken(cookie);
  } catch (const StandardException& e) {
    logDebug("analyticsd", e, "authentication failed because of error");
    return None<AnalyticsSession>();
  }
}

}
