/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include <stx/stdtypes.h>
#include <stx/http/httpservice.h>
#include <stx/json/json.h>
#include <zbase/AnalyticsSession.pb.h>
#include <zbase/AnalyticsAuth.h>

using namespace stx;
namespace zbase {

class WebUIServlet : public stx::http::HTTPService {
public:

  WebUIServlet(AnalyticsAuth* auth);

  void handleHTTPRequest(
      http::HTTPRequest* request,
      http::HTTPResponse* response) override;

protected:

  void handle(
      http::HTTPRequest* request,
      http::HTTPResponse* response);

  String loadFile(const String& filename);

  void renderConfig(
      const Option<AnalyticsSession>& session,
      json::JSONOutputStream* json);

  AnalyticsAuth* auth_;
};

}
