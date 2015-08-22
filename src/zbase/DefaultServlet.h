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

using namespace stx;
namespace zbase {

class DefaultServlet : public stx::http::HTTPService {
public:

  void handleHTTPRequest(
      http::HTTPRequest* request,
      http::HTTPResponse* response) override;

};

}
