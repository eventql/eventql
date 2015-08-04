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
#include "master/CustomerDirectoryMaster.h"

using namespace stx;

namespace cm {

class MasterServlet : public stx::http::HTTPService {
public:

  MasterServlet(CustomerDirectoryMaster* cdb);

  void handleHTTPRequest(
      stx::http::HTTPRequest* req,
      stx::http::HTTPResponse* res) override;

protected:

  void listHeads(
      const URI& uri,
      http::HTTPRequest* request,
      http::HTTPResponse* response);

  void fetchCustomerConfig(
      const URI& uri,
      http::HTTPRequest* req,
      http::HTTPResponse* res);

  void createCustomer(
      const URI& uri,
      http::HTTPRequest* request,
      http::HTTPResponse* response);

  CustomerDirectoryMaster* cdb_;
};

}
