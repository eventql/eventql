/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
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
#include "eventql/util/http/httpservice.h"
#include "eventql/master/ConfigDirectoryMaster.h"

using namespace stx;

namespace eventql {

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
