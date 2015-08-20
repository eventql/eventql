/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <zbase/webui/WebUIServlet.h>
#include <zbase/HTTPAuth.h>

namespace zbase {

WebUIServlet::WebUIServlet(
    AnalyticsAuth* auth) :
    auth_(auth) {}

void WebUIServlet::handleHTTPRequest(
    http::HTTPRequest* request,
    http::HTTPResponse* response) {
  URI uri(request->uri());

  auto session = HTTPAuth::authenticateRequest(*request, auth_);

  static const String kAssetsPathPrefix = "/a/__assets__/";
  if (StringUtil::beginsWith(uri.path(), kAssetsPathPrefix)) {
    auto asset_path = uri.path().substr(kAssetsPathPrefix.size());

    // FIXME validate path

    response->setStatus(http::kStatusOK);
    //response->addHeader("Content-Type", "text/html; charset=utf-8");
    response->addBody(loadFile("assets/" + asset_path)); // FIXME
    return;
  }

  static const String kModulesPathPrefix = "/a/__modules__/";
  if (StringUtil::beginsWith(uri.path(), kModulesPathPrefix)) {
    auto module_name = uri.path().substr(kModulesPathPrefix.size());
    iputs("serve module: $0", module_name);
    response->setStatus(http::kStatusOK);
    response->addHeader("Content-Type", "text/html; charset=utf-8");
    response->addBody(
        FileUtil::read("src/zbase/webui/modules/" +  module_name + ".html").toString()); // FIXME
    return;
  }

  auto app_html = loadFile("app.html");
  StringUtil::replaceAll(&app_html, "{{app_css}}", loadFile("app.css"));
  StringUtil::replaceAll(&app_html, "{{app_js}}", loadFile("app.js"));

  response->setStatus(http::kStatusOK);
  response->addHeader("Content-Type", "text/html; charset=utf-8");
  response->addBody(app_html);
}

String WebUIServlet::loadFile(const String& filename) {
  return FileUtil::read("src/zbase/webui/" + filename).toString();
}

}
