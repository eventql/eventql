/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License ("the license") as
 * published by the Free Software Foundation, either version 3 of the License,
 * or any later version.
 *
 * In accordance with Section 7(e) of the license, the licensing of the Program
 * under the license does not imply a trademark license. Therefore any rights,
 * title and interest in our trademarks remain entirely with us.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the license for more details.
 *
 * You can be released from the requirements of the license by purchasing a
 * commercial license. Buying such a license is mandatory as soon as you develop
 * commercial activities involving this program without disclosing the source
 * code of your own applications
 */
#include <eventql/transport/http/http_auth.h>
#include "eventql/util/http/cookies.h"

namespace eventql {

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
