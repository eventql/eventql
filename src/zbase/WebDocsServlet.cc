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

  static const String kDocumentationAjaxPathPrefix = "/docs/_/d/";
  if (StringUtil::beginsWith(uri.path(), kDocumentationAjaxPathPrefix)) {
    auto asset_path = uri.path().substr(kDocumentationAjaxPathPrefix.size());

    // FIXME validate path
    response->setStatus(http::kStatusOK);
    //response->addHeader("Content-Type", "text/html; charset=utf-8");
    response->addBody(Assets::getAsset("zbase/docs/" + asset_path)); // FIXME
    return;
  }

  static const String kDocumentationPathPrefix = "/docs/";
  if (!StringUtil::beginsWith(uri.path(), kDocumentationPathPrefix)) {
    RAISE(kRuntimeError, "invalid path");
  }

  auto page_path = uri.path().substr(kDocumentationPathPrefix.size());
  String content;
  String title;
  try {
    content = Assets::getAsset("zbase/docs/" + page_path + ".html");
  } catch (const StandardException& e) {
    title = page_path;
    content = StringUtil::format("<h1>page not found: $0</h1>", page_path);
  }

  auto app_html = loadFile("documentation.html");
  StringUtil::replaceAll(&app_html, "{{app_css}}", loadFile("documentation.css"));
  StringUtil::replaceAll(&app_html, "{{app_js}}", loadFile("documentation.js"));
  StringUtil::replaceAll(&app_html, "{{doc_content}}", content);
  StringUtil::replaceAll(&app_html, "{{doc_title}}", title);

  response->setStatus(http::kStatusOK);
  response->addHeader("Content-Type", "text/html; charset=utf-8");
  response->addBody(app_html);
}

String WebDocsServlet::loadFile(const String& filename) {
  return Assets::getAsset("zbase/webdocs/" + filename);
}

}
