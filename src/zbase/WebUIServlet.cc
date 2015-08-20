/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <stx/protobuf/msg.h>
#include <zbase/WebUIServlet.h>
#include <zbase/WebUIModuleConfig.pb.h>
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

  static const String kAssetsPathPrefix = "/a/__/";
  if (StringUtil::beginsWith(uri.path(), kAssetsPathPrefix)) {
    auto asset_path = uri.path().substr(kAssetsPathPrefix.size());

    // FIXME validate path

    //"css" => "text/css; charset=utf-8",
    //"png" => "image/png",
    //"html" => "text/html; charset=utf-8",
    //"js" => "application/javascript; charset=utf-8"
    response->setStatus(http::kStatusOK);
    //response->addHeader("Content-Type", "text/html; charset=utf-8");
    response->addBody(loadFile("assets/" + asset_path)); // FIXME
    return;
  }

  static const String kModulesPathPrefix = "/a/_/";
  if (StringUtil::beginsWith(uri.path(), kModulesPathPrefix)) {
    auto module_name = uri.path().substr(kModulesPathPrefix.size());

    // FIXME cache module config
    auto module_cfg = msg::parseText<WebUIModuleConfig>(
        loadFile(StringUtil::format("modules/$0/MANIFEST", module_name)));

    // FIXME check that module if module is public/private

    String module_html;

    for (const auto& file : module_cfg.html_file()) {
      module_html +=
          loadFile(StringUtil::format("modules/$0/$1", module_name, file));
    }

    for (const auto& file : module_cfg.css_file()) {
      module_html += StringUtil::format(
        "<style type='text/css'>$0</style>",
        loadFile(StringUtil::format("modules/$0/$1", module_name, file)));
    }

    for (const auto& file : module_cfg.js_file()) {
      module_html += StringUtil::format(
        "<script type='text/javascript'>$0</script>",
        loadFile(StringUtil::format("modules/$0/$1", module_name, file)));
    }

    module_html += StringUtil::format(
        "<script type='text/javascript'>ZBase.moduleReady('$0');</script>",
        module_name);

    response->setStatus(http::kStatusOK);
    response->addHeader("Content-Type", "text/html; charset=utf-8");
    response->addBody(module_html);
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
