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
#include "eventql/master/MasterServlet.h"
#include "eventql/util/io/outputstream.h"
#include "eventql/util/io/BufferedOutputStream.h"
#include "eventql/util/logging.h"
#include "eventql/util/protobuf/msg.h"
#include "eventql/util/random.h"

using namespace stx;

namespace eventql {

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

    if (StringUtil::endsWith(uri.path(), "/analytics/master/fetch_cluster_config")) {
      return fetchClusterConfig(uri, req, res);
    }

    if (StringUtil::endsWith(uri.path(), "/analytics/master/update_cluster_config")) {
      return updateClusterConfig(uri, req, res);
    }

    if (StringUtil::endsWith(uri.path(), "/analytics/master/fetch_customer_config")) {
      return fetchCustomerConfig(uri, req, res);
    }

    if (StringUtil::endsWith(uri.path(), "/analytics/master/update_customer_config")) {
      return updateCustomerConfig(uri, req, res);
    }

    if (StringUtil::endsWith(uri.path(), "/analytics/master/fetch_userdb")) {
      return fetchUserDB(uri, req, res);
    }

    if (StringUtil::endsWith(uri.path(), "/analytics/master/create_user")) {
      return createUser(uri, req, res);
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

void MasterServlet::fetchClusterConfig(
    const URI& uri,
    http::HTTPRequest* req,
    http::HTTPResponse* res) {
  auto config = cdb_->fetchClusterConfig();
  auto body = msg::encode(config);

  res->setStatus(stx::http::kStatusOK);
  res->addBody(*body);
}

void MasterServlet::updateClusterConfig(
    const URI& uri,
    http::HTTPRequest* req,
    http::HTTPResponse* res) {
  if (req->method() != http::HTTPMessage::M_POST) {
    RAISE(kIllegalArgumentError, "expected HTTP POST request");
  }

  auto config = msg::decode<ClusterConfig>(req->body());
  auto updated_config = cdb_->updateClusterConfig(config);
  res->setStatus(stx::http::kStatusCreated);
  res->addBody(*msg::encode(updated_config));
}

void MasterServlet::fetchUserDB(
    const URI& uri,
    http::HTTPRequest* req,
    http::HTTPResponse* res) {
  const auto& params = uri.queryParams();

  auto users = cdb_->fetchUserDB();
  auto body = msg::encode(users);

  res->setStatus(stx::http::kStatusOK);
  res->addBody(*body);
}

void MasterServlet::createUser(
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

  String userid;
  if (!stx::URI::getParam(params, "userid", &userid)) {
    res->addBody("error: missing ?userid=... parameter");
    res->setStatus(http::kStatusBadRequest);
    return;
  }

  String password;
  if (!stx::URI::getParam(params, "password", &password)) {
    res->addBody("error: missing ?password=... parameter");
    res->setStatus(http::kStatusBadRequest);
    return;
  }

  auto pwsalt = Random::singleton()->hex128();
  auto pwhash = SHA1::compute(pwsalt + password);

  UserConfig user;
  user.set_customer(customer);
  user.set_userid(userid);
  user.set_password_hash(pwhash.toString());
  user.set_password_salt(pwsalt);
  user.set_two_factor_method(USERAUTH_2FA_NONE);

  String force;
  if (stx::URI::getParam(params, "force_reset", &force) && force == "true") {
    auto head_cfg = cdb_->fetchUserConfig(userid);
    user.set_version(head_cfg.version());
  }

  cdb_->updateUserConfig(user);
  res->setStatus(stx::http::kStatusCreated);
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
  if (stx::URI::getParam(params, "force_reset", &force) && force == "true") {
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
  const auto& params = uri.queryParams();

  if (req->method() != http::HTTPMessage::M_POST) {
    RAISE(kIllegalArgumentError, "expected HTTP POST request");
  }

  bool force = false;
  String force_str;
  if (stx::URI::getParam(params, "force", &force_str) && force_str == "true") {
    force = true;
  }

  auto tbl = msg::decode<TableDefinition>(req->body());
  auto updated_tbl = cdb_->updateTableDefinition(tbl, force);
  res->setStatus(stx::http::kStatusCreated);
  res->addBody(*msg::encode(updated_tbl));
}

}
