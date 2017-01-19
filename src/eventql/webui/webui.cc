/**
 * Copyright (c) 2017 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#include <eventql/webui/webui.h>
#include <metricd/util/logging.h>
#include <eventql/util/io/fileutil.h>

namespace eventql {

WebUI::WebUI(
    const std::string& dynamic_asset_path /* = "" */) :
    dynamic_asset_path_(dynamic_asset_path) {}

void WebUI::handleHTTPRequest(
    http::HTTPRequest* request,
    http::HTTPResponse* response) {

  logDebug(
      "HTTP request: $0 $1",
      http::getHTTPMethodName(request->method()),
      request->uri());

  URI uri(request->uri());
  const auto& path = uri.path();

  if (path == "/") {
    response->setStatus(http::kStatusFound);
    response->addHeader("Location", "/ui/");
    return;
  }

  if (StringUtil::beginsWith(path, "/ui/")) {
    response->setStatus(http::kStatusOK);
    response->addHeader("Content-Type", "text/html; charset=utf-8");
    response->addBody(getPreludeHTML());
    return;
  }

  if (StringUtil::beginsWith(path, "/assets/app.html")) {
    response->setStatus(http::kStatusOK);
    response->addHeader("Content-Type", "text/html; charset=utf-8");
    response->addBody(getAppHTML());
    return;
  }

  if (path == "/favicon.ico") {
    response->setStatus(http::kStatusOK);
    response->addHeader("Content-Type", "image/x-icon");
    response->addBody(getAssetFile("favicon.ico"));
    return;
  }

  if (path == "/fontawesome.woff") {
    response->setStatus(http::kStatusOK);
    response->addHeader("Content-Type", "application/x-font-woff");
    response->addBody(getAssetFile("fontawesome.woff"));
    return;
  }

  response->setStatus(http::kStatusNotFound);
  response->addHeader("Content-Type", "text/plain; charset=utf-8");
  response->addBody("not found");
}

std::string WebUI::getPreludeHTML() const {
  return getAssetFile("prelude.html");
}

std::string WebUI::getAppHTML() const {
  auto assets_lst = FileUtil::read(
      FileUtil::joinPaths(dynamic_asset_path_, "assets.lst")).toString();

  std::string app_html;
  for (const auto& f : StringUtil::split(assets_lst, "\n")) {
    auto file_path = FileUtil::joinPaths(dynamic_asset_path_, f);
    if (!FileUtil::exists(file_path)) {
      continue;
    }

    if (StringUtil::endsWith(f, ".html")) {
      app_html += FileUtil::read(file_path).toString();
    }

    if (StringUtil::endsWith(f, ".js")) {
      auto content = FileUtil::read(file_path).toString();
      app_html += "<script>" + content + "</script>";
    }

    if (StringUtil::endsWith(f, ".css")) {
      auto content = FileUtil::read(file_path).toString();
      app_html += "<style type='text/css'>" + content + "</style>";
    }
  }

  return app_html;
}

std::string WebUI::getAssetFile(const std::string& file) const {
  if (!dynamic_asset_path_.empty()) {
    auto file_path = FileUtil::joinPaths(dynamic_asset_path_, file);
    if (FileUtil::exists(file_path)) {
      return FileUtil::read(file_path).toString();
    }
  }

  return "";
}

} //namespace eventql

