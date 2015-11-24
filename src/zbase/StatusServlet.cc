/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <zbase/StatusServlet.h>

using namespace stx;
namespace zbase {

void StatusServlet::handleHTTPRequest(
    http::HTTPRequest* request,
    http::HTTPResponse* response) {
    http::HTTPResponse res;
  String html = "blah";

  response->setStatus(http::kStatusOK);
  response->addHeader("Content-Type", "text/html; charset=utf-8");
  response->addBody(html);
}

}
