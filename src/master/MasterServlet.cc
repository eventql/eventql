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
#include "stx/protobuf/msg.h"

using namespace stx;

namespace cm {

MasterServlet::MasterServlet(
    ConfigDirectoryMaster* cdb) :
    cdb_(cdb) {}

void MasterServlet::handleHTTPRequest(
    http::HTTPRequest* req,
    http::HTTPResponse* res) {
  try {
    URI uri(req->uri());

    if (StringUtil::endsWith(uri.path(), "/analytics/master/heads")) {
      return listHeads(uri, req, res);
    }

    if (StringUtil::endsWith(uri.path(), "/analytics/master/fetch_customer_config")) {
      return fetchCustomerConfig(uri, req, res);
    }

    if (StringUtil::endsWith(uri.path(), "/analytics/master/update_customer_config")) {
      return updateCustomerConfig(uri, req, res);
    }

    if (StringUtil::endsWith(uri.path(), "/analytics/master/create_customer")) {
      return createCustomer(uri, req, res);
    }

    if (StringUtil::endsWith(uri.path(), "/analytics/master/fetch_table_definition")) {
      return fetchTableDefinition(uri, req, res);
    }

    if (StringUtil::endsWith(uri.path(), "/analytics/master/fetch_table_definitions")) {
      return fetchTableDefinitions(uri, req, res);
    }

    if (StringUtil::endsWith(uri.path(), "/analytics/master/update_table_definition")) {
      return updateTableDefinition(uri, req, res);
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

void MasterServlet::fetchCustomerConfig(
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

  auto config = cdb_->fetchCustomerConfig(customer);
  auto body = msg::encode(config);

  res->setStatus(stx::http::kStatusOK);
  res->addBody(*body);
}

void MasterServlet::updateCustomerConfig(
    const URI& uri,
    http::HTTPRequest* req,
    http::HTTPResponse* res) {
  if (req->method() != http::HTTPMessage::M_POST) {
    RAISE(kIllegalArgumentError, "expected HTTP POST request");
  }

  auto config = msg::decode<CustomerConfig>(req->body());
  auto updated_config = cdb_->updateCustomerConfig(config);
  res->setStatus(stx::http::kStatusCreated);
  res->addBody(*msg::encode(updated_config));
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

  auto cfg = createCustomerConfig(customer);

  String force;
  if (stx::URI::getParam(params, "force_reset", &force) && force == "YES") {
    auto head_cfg = cdb_->fetchCustomerConfig(customer);
    cfg.set_version(head_cfg.version());
  }

  cdb_->updateCustomerConfig(cfg);
  res->setStatus(stx::http::kStatusCreated);
}

void MasterServlet::fetchTableDefinition(
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

  String table_name;
  if (!stx::URI::getParam(params, "table_name", &table_name)) {
    res->addBody("error: missing ?table_name=... parameter");
    res->setStatus(http::kStatusBadRequest);
    return;
  }

  auto td = cdb_->fetchTableDefinition(customer, table_name);
  auto body = msg::encode(td);

  res->setStatus(stx::http::kStatusOK);
  res->addBody(*body);
}

void MasterServlet::fetchTableDefinitions(
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

  auto tables = cdb_->fetchTableDefinitions(customer);
  auto body = msg::encode(tables);

  res->setStatus(stx::http::kStatusOK);
  res->addBody(*body);
}

void MasterServlet::updateTableDefinition(
    const URI& uri,
    http::HTTPRequest* req,
    http::HTTPResponse* res) {
  if (req->method() != http::HTTPMessage::M_POST) {
    RAISE(kIllegalArgumentError, "expected HTTP POST request");
  }

  auto tbl = msg::decode<TableDefinition>(req->body());
  auto updated_tbl = cdb_->updateTableDefinition(tbl);
  res->setStatus(stx::http::kStatusCreated);
  res->addBody(*msg::encode(updated_tbl));
}

}
