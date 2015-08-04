/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "master/CustomerDirectoryServlet.h"

using namespace stx;

namespace cm {


CustomerDirectoryServlet::CustomerDirectoryServlet(
    CustomerDirectory* cdb) : 
    cdb_(cdb) {}

void CustomerDirectoryServlet::handleHTTPRequest(
    http::HTTPRequest* req,
    http::HTTPResponse* res) {
  URI uri(req->uri());

  if (StringUtil::endsWith(uri.path(), "/cdb/create_customer")) {
    return createCustomer(uri, req, res);
  }

  res->setStatus(stx::http::kStatusNotFound);
  res->addBody("not found");
}

void CustomerDirectoryServlet::createCustomer(
    const URI& uri,
    http::HTTPRequest* req,
    http::HTTPResponse* res) {
  const auto& params = uri.queryParams();

  String customer;
  if (!stx::URI::getParam(params, "customer", &customer)) {
    res->addBody("error: missing ?customer=... parameter");
    res->setStatus(http::kStatusBadRequest);
    return;
  }

  cdb_->updateCustomerConfig(createCustomerConfig(customer));
}

}
