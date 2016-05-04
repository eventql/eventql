/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include "eventql/util/http/httpservice.h"
#include "eventql/master/ConfigDirectoryMaster.h"

using namespace stx;

namespace zbase {

class MasterServlet : public stx::http::HTTPService {
public:

  MasterServlet(ConfigDirectoryMaster* cdb);

  void handleHTTPRequest(
      stx::http::HTTPRequest* req,
      stx::http::HTTPResponse* res) override;

protected:

  void listHeads(
      const URI& uri,
      http::HTTPRequest* request,
      http::HTTPResponse* response);

  void fetchClusterConfig(
      const URI& uri,
      http::HTTPRequest* req,
      http::HTTPResponse* res);

  void updateClusterConfig(
      const URI& uri,
      http::HTTPRequest* request,
      http::HTTPResponse* response);

  void fetchCustomerConfig(
      const URI& uri,
      http::HTTPRequest* req,
      http::HTTPResponse* res);

  void updateCustomerConfig(
      const URI& uri,
      http::HTTPRequest* request,
      http::HTTPResponse* response);

  void createCustomer(
      const URI& uri,
      http::HTTPRequest* request,
      http::HTTPResponse* response);

  void fetchTableDefinition(
      const URI& uri,
      http::HTTPRequest* req,
      http::HTTPResponse* res);

  void fetchTableDefinitions(
      const URI& uri,
      http::HTTPRequest* req,
      http::HTTPResponse* res);

  void updateTableDefinition(
      const URI& uri,
      http::HTTPRequest* req,
      http::HTTPResponse* res);

  void fetchUserDB(
      const URI& uri,
      http::HTTPRequest* req,
      http::HTTPResponse* res);

  void createUser(
      const URI& uri,
      http::HTTPRequest* req,
      http::HTTPResponse* res);

  ConfigDirectoryMaster* cdb_;
};

}
