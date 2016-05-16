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
#include <eventql/util/http/cookies.h>
#include <eventql/util/assets.h>
#include <eventql/transport/http/default_servlet.h>
#include <eventql/transport/http/http_auth.h>

namespace eventql {

void DefaultServlet::handleHTTPRequest(
    http::HTTPRequest* request,
    http::HTTPResponse* response) {
  URI uri(request->uri());

  if (uri.path() == "/" &&
      request->hasHeader("X-ZenBase-Production")) {
    String auth_cookie;
    http::Cookies::getCookie(
        request->cookies(),
        HTTPAuth::kSessionCookieKey,
        &auth_cookie);

    if (auth_cookie.empty()) {
      response->setStatus(http::kStatusFound);
      response->addHeader("Location", "http://app.eventql.io/");
      return;
    }
  }

  if (uri.path() == "/") {
    response->setStatus(http::kStatusFound);
    response->addHeader("Location", "/a/");
    return;
  }

  if (uri.path() == "/ping") {
    response->setStatus(http::kStatusOK);
    response->addBody("pong");
    return;
  }

  response->setStatus(http::kStatusNotFound);
  response->addHeader("Content-Type", "text/plain; charset=utf-8");
  response->addBody("not found");
}

}
