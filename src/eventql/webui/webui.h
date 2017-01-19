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
#pragma once
#include <memory>
#include <eventql/util/http/http_request.h>
#include <eventql/util/http/http_response.h>
#include <eventql/util/uri.h>

namespace eventql {

class WebUI {
public:

  WebUI(const std::string& dynamic_asset_path = "");

  void handleHTTPRequest(
      http::HTTPRequest* request,
      http::HTTPResponse* response);

private:

  std::string getPreludeHTML() const;
  std::string getAppHTML() const;

  std::string getAssetFile(const std::string& file) const;

  void sendAsset(
      http::HTTPResponse* response,
      const std::string& asset_path,
      const std::string& content_type) const;

  std::string dynamic_asset_path_;
};

} //namespace eventql

