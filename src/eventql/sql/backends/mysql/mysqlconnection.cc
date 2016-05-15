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
//#include <fnordmetric/environment.h>
#include <eventql/util/logging.h>
#include <eventql/sql/backends/mysql/mysqlconnection.h>

namespace csql {
namespace mysql_backend {

std::unique_ptr<MySQLConnection> MySQLConnection::openConnection(
    const URI& uri) {
  std::unique_ptr<MySQLConnection> conn(new MySQLConnection());
  conn->connect(uri);
  return conn;
}

MySQLConnection::MySQLConnection() : mysql_(nullptr) {
#ifdef FNORD_ENABLE_MYSQL
  mysql_ = mysql_init(NULL);

  if (mysql_ == nullptr) {
    RAISE(kRuntimeError, "mysql_init() failed\n");
  }
#else
  RAISE(kRuntimeError, "FnordMetric was compiled without libmysqlclient");
#endif
}

MySQLConnection::~MySQLConnection() {
#ifdef FNORD_ENABLE_MYSQL
  mysql_close(mysql_);
#else
  RAISE(kRuntimeError, "FnordMetric was compiled without libmysqlclient");
#endif
}

void MySQLConnection::connect(const URI& uri) {
  unsigned int port = 3306;
  std::string host = uri.host();
  std::string username;
  std::string password;
  std::string database;

  if (host.size() == 0) {
    RAISE(
        kRuntimeError,
        "invalid mysql:// URI: has no hostname (URI: '%s')",
        uri.toString().c_str());
  }

  if (uri.port() > 0) {
    port = uri.port();
  }

  if (uri.path().size() < 2 || uri.path()[0] != '/') {
    RAISE(
        kRuntimeError,
        "invalid mysql:// URI: missing database, format is: mysql://host/db "
        " (URI: %s)",
        uri.toString().c_str());
  }

  database = uri.path().substr(1);

  for (const auto& param : uri.queryParams()) {
    if (param.first == "username" || param.first == "user") {
      username = param.second;
      continue;
    }

    if (param.first == "password" || param.first == "pass") {
      password = param.second;
      continue;
    }

    RAISE(
        kRuntimeError,
        "invalid parameter for mysql:// URI: '%s=%s'",
        param.first.c_str(),
        param.second.c_str());
  }

  connect(host, port, database, username, password);
}

void MySQLConnection::connect(
    const std::string& host,
    unsigned int port,
    const std::string& database,
    const std::string& username,
    const std::string& password) {
#ifdef FNORD_ENABLE_MYSQL
  auto ret = mysql_real_connect(
      mysql_,
      host.c_str(),
      username.size() > 0 ? username.c_str() : NULL,
      password.size() > 0 ? password.c_str() : NULL,
      database.size() > 0 ? database.c_str() : NULL,
      port,
      NULL,
      CLIENT_COMPRESS);

  if (ret != mysql_) {
    RAISE(
      kRuntimeError,
      "mysql_real_connect() failed: %s\n",
      mysql_error(mysql_));
  }
#else
  RAISE(kRuntimeError, "FnordMetric was compiled without libmysqlclient");
#endif
}


std::vector<std::string> MySQLConnection::describeTable(
    const std::string& table_name) {
  std::vector<std::string> columns;

#ifdef FNORD_ENABLE_MYSQL
  MYSQL_RES* res = mysql_list_fields(mysql_, table_name.c_str(), NULL);
  if (res == nullptr) {
    RAISE(
      kRuntimeError,
      "mysql_list_fields() failed: %s\n",
      mysql_error(mysql_));
  }

  auto num_cols = mysql_num_fields(res);
  for (int i = 0; i < num_cols; ++i) {
    MYSQL_FIELD* col = mysql_fetch_field_direct(res, i);
    columns.emplace_back(col->name);
  }

  mysql_free_result(res);
#else
  RAISE(kRuntimeError, "FnordMetric was compiled without libmysqlclient");
#endif
  return columns;
}

void MySQLConnection::executeQuery(
    const std::string& query,
    std::function<bool (const std::vector<std::string>&)> row_callback) {
#ifdef FNORD_ENABLE_MYSQL
  if (env()->verbose()) {
    util::logDebug("fnordmetric", "Executing MySQL query: $0", query);
  }

  MYSQL_RES* result = nullptr;
  if (mysql_real_query(mysql_, query.c_str(), query.size()) == 0) {
    result = mysql_use_result(mysql_);
  }

  if (result == nullptr) {
    RAISE(
        kRuntimeError,
        "mysql query failed: %s -- error: %s\n",
        query.c_str(),
        mysql_error(mysql_));
  }


  MYSQL_ROW row;
  while ((row = mysql_fetch_row(result))) {
    auto col_lens = mysql_fetch_lengths(result);
    if (col_lens == nullptr) {
      break;
    }

    std::vector<std::string> row_vec;
    auto row_len = mysql_num_fields(result);
    for (int i = 0; i < row_len; ++i) {
      row_vec.emplace_back(row[i], col_lens[i]);
    }

    try {
      if (!row_callback(row_vec)) {
        break;
      }
    } catch (const std::exception& e) {
      mysql_free_result(result);
      try {
        auto rte = dynamic_cast<const Exception&>(e);
        throw rte;
      } catch (std::bad_cast bce) {
        throw e;
      }
    }
  }

  mysql_free_result(result);
#else
  RAISE(kRuntimeError, "FnordMetric was compiled without libmysqlclient");
#endif
}

}
}
