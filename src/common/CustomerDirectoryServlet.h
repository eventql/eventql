/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include "stx/http/httpservice.h"
#include "common/CustomerDirectory.h"

using namespace stx;

namespace cm {

class CustomerDirectoryServlet : public stx::http::HTTPService {
public:

  CustomerDirectoryServlet(CustomerDirectory* cdb);

  void handleHTTPRequest(
      stx::http::HTTPRequest* req,
      stx::http::HTTPResponse* res) override;

protected:

  void createCustomer(
      const URI& uri,
      http::HTTPRequest* request,
      http::HTTPResponse* response);

  CustomerDirectory* cdb_;
};

}
