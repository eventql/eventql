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
#include <zbase/WebDocsServlet.h>
#include <zbase/WebUIModuleConfig.pb.h>
#include <zbase/HTTPAuth.h>
#include <zbase/buildconfig.h>

namespace zbase {

void WebDocsServlet::handleHTTPRequest(
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

void WebDocsServlet::handle(
    http::HTTPRequest* request,
    http::HTTPResponse* response) {
  URI uri(request->uri());

  static const String kDocumentationPathPrefix = "/docs/_/d/";
  if (StringUtil::beginsWith(uri.path(), kDocumentationPathPrefix)) {
    auto asset_path = uri.path().substr(kDocumentationPathPrefix.size());

    // FIXME validate path
    response->setStatus(http::kStatusOK);
    //response->addHeader("Content-Type", "text/html; charset=utf-8");
    response->addBody(Assets::getAsset("zbase/docs/" + asset_path)); // FIXME
    return;
  }

  //Buffer app_config;
  //json::JSONOutputStream app_config_json(
  //    BufferOutputStream::fromBuffer(&app_config));
  //renderConfig(request, session, &app_config_json);

  auto app_html = loadFile("documentation.html");
  StringUtil::replaceAll(&app_html, "{{app_css}}", loadFile("documentation.css"));
  StringUtil::replaceAll(&app_html, "{{app_js}}", loadFile("documentation.js"));
  //StringUtil::replaceAll(&app_html, "{{config_json}}", app_config.toString());
  StringUtil::replaceAll(&app_html, "{{doc_content}}", Assets::getAsset("zbase/docs/async_session_tracking.html"));


  response->setStatus(http::kStatusOK);
  response->addHeader("Content-Type", "text/html; charset=utf-8");
  response->addBody(app_html);
}

String WebDocsServlet::loadFile(const String& filename) {
  return Assets::getAsset("zbase/webdocs/" + filename);
}

}
