/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include <stx/protobuf/msg.h>
#include <stx/json/json.h>
#include <zbase/WebUIServlet.h>
#include <zbase/WebUIModuleConfig.pb.h>
#include <zbase/HTTPAuth.h>
#include <zbase/buildconfig.h>

namespace zbase {

WebUIServlet::WebUIServlet(
    AnalyticsAuth* auth) :
    auth_(auth) {}

void WebUIServlet::handleHTTPRequest(
    http::HTTPRequest* request,
    http::HTTPResponse* response) {
  URI uri(request->uri());
  try {
    handle(request, response);
  } catch (const StandardException& e) {
    logError("zbase", e, "error while handling HTTP request");
    response->setStatus(http::kStatusInternalServerError);
    response->addHeader("Content-Type", "text/html; charset=utf-8");
    response->addBody(Assets::getAsset("zbase/webui/500.html"));
  }
}

void WebUIServlet::handle(
    http::HTTPRequest* request,
    http::HTTPResponse* response) {
  URI uri(request->uri());

  auto session = HTTPAuth::authenticateRequest(*request, auth_);

  static const String kAssetsPathPrefix = "/a/_/a/";
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

  static const String kConfigPath = "/a/_/c";
  if (uri.path() == kConfigPath) {
    Buffer buf;
    json::JSONOutputStream json(BufferOutputStream::fromBuffer(&buf));

    renderConfig(request, session, &json);

    response->setStatus(http::kStatusOK);
    response->addHeader("Content-Type", "application/json; charset=utf-8");
    response->addBody(buf);
    return;
  }

  static const String kModulesPathPrefix = "/a/_/m/";
  if (StringUtil::beginsWith(uri.path(), kModulesPathPrefix)) {
    auto module_name = uri.path().substr(kModulesPathPrefix.size());

    // FIXME cache module config
    auto module_cfg = msg::parseText<WebUIModuleConfig>(
        loadFile(StringUtil::format("modules/$0/MANIFEST", module_name)));

    if (session.isEmpty() && !module_cfg.is_public()) {
      response->setStatus(http::kStatusForbidden);
      response->addHeader("Content-Type", "text/html; charset=utf-8");
      response->addBody(Assets::getAsset("zbase/webui/403.html"));
      return;
    }

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

  Buffer app_config;
  json::JSONOutputStream app_config_json(
      BufferOutputStream::fromBuffer(&app_config));
  renderConfig(request, session, &app_config_json);

  auto app_html = loadFile("app.html");
  StringUtil::replaceAll(&app_html, "{{app_css}}", loadFile("app.css"));
  StringUtil::replaceAll(&app_html, "{{app_js}}", loadFile("app.js"));
  StringUtil::replaceAll(&app_html, "{{config_json}}", app_config.toString());

  response->setStatus(http::kStatusOK);
  response->addHeader("Content-Type", "text/html; charset=utf-8");
  response->addBody(app_html);
}

String WebUIServlet::loadFile(const String& filename) {
  return Assets::getAsset("zbase/webui/" + filename);
}

void WebUIServlet::renderConfig(
    http::HTTPRequest* request,
    const Option<AnalyticsSession>& session,
    json::JSONOutputStream* json) {
  auto app_cfg = msg::parseText<WebUIAppConfig>(loadFile("MANIFEST"));

  json->beginObject();

  // routes
  json->addObjectEntry("routes");
  json->beginArray();

  size_t nroute = 0;
  for (const auto& route : app_cfg.route()) {
    if (!route.is_public() && session.isEmpty()) {
      continue;
    }

    Vector<String> modules(
        route.require_module().begin(),
        route.require_module().end());

    if (++nroute > 1) {
      json->addComma();
    }

    json->beginObject();

    json->addObjectEntry("view");
    json->addString(route.view());

    if (route.has_path_prefix()) {
      json->addComma();
      json->addObjectEntry("path_prefix");
      json->addString(route.path_prefix());
    }

    if (route.has_path_match()) {
      json->addComma();
      json->addObjectEntry("path_match");
      json->addString(route.path_match());
    }

    json->addComma();
    json->addObjectEntry("modules");
    json::toJSON(modules, json);

    json->endObject();
  }

  json->endArray();

  // current user
  if (!session.isEmpty()) {
    json->addComma();
    json->addObjectEntry("current_user");
    json->beginObject();

    json->addObjectEntry("userid");
    json->addString(session.get().userid());
    json->addComma();

    json->addObjectEntry("namespace");
    json->addString(session.get().customer());

    json->endObject();
  }

  // default route
  json->addComma();
  json->addObjectEntry("default_route");
  json->addString(session.isEmpty() ? "/a/login" : "/a/");
  json->addComma();

  json->addObjectEntry("zbase_domain");
  json->addString(getDomain(*request));
  json->addComma();

  json->addObjectEntry("zbase_build_id");
  json->addString(ZBASE_BUILD_ID);

  json->endObject();
}

}
