/**
 * Copyright (c) 2015 - The CM Authors <legal@clickmatcher.com>
 *   All Rights Reserved.
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#include "master/MasterServlet.h"
#include "stx/io/outputstream.h"
#include "stx/io/BufferedOutputStream.h"
#include "stx/logging.h"

using namespace stx;

namespace cm {

MasterServlet::MasterServlet(
    CustomerDirectoryMaster* cdb) :
    cdb_(cdb) {}

void MasterServlet::handleHTTPRequest(
    http::HTTPRequest* req,
    http::HTTPResponse* res) {
  try {
    URI uri(req->uri());

    if (StringUtil::endsWith(uri.path(), "/analytics/master/heads")) {
      return listHeads(uri, req, res);
    }

    if (StringUtil::endsWith(uri.path(), "/analytics/master/create_customer")) {
      return createCustomer(uri, req, res);
    }
  } catch (const StandardException& e) {
    logError("dxa-master", e, "error while handling HTTP request");
    res->setStatus(stx::http::kStatusInternalServerError);
    res->addBody(StringUtil::format("error: $0", e.what()));
    return;
  }

  res->setStatus(stx::http::kStatusNotFound);
  res->addBody("not found");
}

void MasterServlet::listHeads(
    const URI& uri,
    http::HTTPRequest* req,
    http::HTTPResponse* res) {
  auto heads = cdb_->heads();

  Buffer body;
  BufferedOutputStream os(BufferOutputStream::fromBuffer(&body));

  for (const auto& h : heads) {
    os.appendString(StringUtil::format("$0=$1\n", h.first, h.second));
  }

  os.flush();

  res->setStatus(stx::http::kStatusOK);
  res->addBody(body);
}

void MasterServlet::createCustomer(
    const URI& uri,
    http::HTTPRequest* req,
    http::HTTPResponse* res) {
  if (req->method() != http::HTTPMessage::M_POST) {
    RAISE(kIllegalArgumentError, "expected HTTP POST request");
  }

  const auto& params = uri.queryParams();

  String customer;
  if (!stx::URI::getParam(params, "customer", &customer)) {
    res->addBody("error: missing ?customer=... parameter");
    res->setStatus(http::kStatusBadRequest);
    return;
  }

  cdb_->updateCustomerConfig(createCustomerConfig(customer));
  res->setStatus(stx::http::kStatusCreated);
}

}
