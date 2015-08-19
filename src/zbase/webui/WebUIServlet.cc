/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <zbase/webui/WebUIServlet.h>

namespace zbase {

void WebUIServlet::handleHTTPRequest(
    http::HTTPRequest* request,
    http::HTTPResponse* response) {
  URI uri(request->uri());

  static const String kAssetsPathPrefix = "/a/__assets__/";
  if (StringUtil::beginsWith(uri.path(), kAssetsPathPrefix)) {
    auto asset_path = uri.path().substr(kAssetsPathPrefix.size());
    iputs("asset path: $0", asset_path);
  }

  static const String kModulesPathPrefix = "/a/__modules__/";
  if (StringUtil::beginsWith(uri.path(), kModulesPathPrefix)) {
    auto module_name = uri.path().substr(kModulesPathPrefix.size());
    iputs("serve module: $0", module_name);
  }

  //serveModule("app_main");

  //if (path == "/") {
  //  response->setStatus(http::kStatusFound);
  //  response->addHeader("Content-Type", "text/html; charset=utf-8");
  //  response->addHeader("Location", path_prefix_);
  //  return;
  //}

  //if (path == "/fontawesome.woff") {
  //  request->setURI(
  //      StringUtil::format(
  //          "$0/__components__/fnord/3rdparty/fontawesome.woff",
  //          path_prefix_));
  //}

  //webui_mount_.handleHTTPRequest(request, response);
  response->setStatus(http::kStatusOK);
  response->addBody("hello world");
}


}
