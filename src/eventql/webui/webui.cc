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
#include "eventql/webui/webui.h"
#include "eventql/util/logging.h"
#include "eventql/util/io/fileutil.h"

namespace eventql {

void WebUIServlet::handleHTTPRequest(
    RefPtr<http::HTTPRequestStream> req_stream,
    RefPtr<http::HTTPResponseStream> res_stream) {

  const auto& req = req_stream->request();
  URI uri(req.uri());

  logDebug("eventql", "HTTP Request: $0 $1", req.method(), req.uri());

  http::HTTPResponse res;
  res.populateFromRequest(req);

  if (StringUtil::beginsWith(uri.path(), "/ui/assets/app.html")) {
    res.setStatus(http::kStatusOK);
    res.addHeader("Content-Type", "text/html; charset=utf-8");
    res.addBody(getAppHTML());
    res_stream->writeResponse(res);
    return;
  }

  if (uri.path() == "/ui/favicon.png") {
    res.setStatus(http::kStatusOK);
    res.addHeader("Content-Type", "image/x-icon");
    res.addBody(getAssetFile("favicon.png"));
    res_stream->writeResponse(res);
    return;
  }

  res.setStatus(http::kStatusOK);
  res.addHeader("Content-Type", "text/html; charset=utf-8");
  res.addBody(getPreludeHTML());
  res_stream->writeResponse(res);
  return;
}

std::string WebUIServlet::getPreludeHTML() const {
  return getAssetFile("prelude.html");
}

std::string WebUIServlet::getAppHTML() const {
  auto assets_lst = FileUtil::read(
      FileUtil::joinPaths(kAssetPath, "assets.lst")).toString();

  std::string app_html;
  for (const auto& f : StringUtil::split(assets_lst, "\n")) {
    auto file_path = FileUtil::joinPaths(kAssetPath, f);
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

std::string WebUIServlet::getAssetFile(const std::string& file) const {
  auto file_path = FileUtil::joinPaths(kAssetPath, file);
  if (FileUtil::exists(file_path)) {
    return FileUtil::read(file_path).toString();
  }

  return "";
}

} //namespace eventql

