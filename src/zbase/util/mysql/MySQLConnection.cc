/**
 * This file is part of the "libstx" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/logging.h>
#include <stx/net/mysql/MySQLConnection.h>

namespace stx {
namespace mysql {

std::unique_ptr<MySQLConnection> MySQLConnection::openConnection(
    const stx::URI& uri) {
  std::unique_ptr<MySQLConnection> conn(new MySQLConnection());
  conn->connect(uri);
  return conn;
}

std::unique_ptr<MySQLConnection> MySQLConnection::openConnection(
    const std::string& host,
    unsigned int port,
    const std::string& database,
    const std::string& username,
    const std::string& password) {
  std::unique_ptr<MySQLConnection> conn(new MySQLConnection());
  conn->connect(host, port, database, username, password);
  return conn;
}

MySQLConnection::MySQLConnection() : mysql_(nullptr) {
#ifdef STX_ENABLE_MYSQL
  mysql_ = mysql_init(NULL);

  if (mysql_ == nullptr) {
    RAISE(kRuntimeError, "mysql_init() failed\n");
  }

  mysql_options(mysql_, MYSQL_SET_CHARSET_NAME, "utf8");
#else
  RAISE(kRuntimeError, "libstx was compiled without libmysqlclient");
#endif
}

MySQLConnection::~MySQLConnection() {
#ifdef STX_ENABLE_MYSQL
  mysql_close(mysql_);
#else
  RAISE(kRuntimeError, "libstx was compiled without libmysqlclient");
#endif
}

void MySQLConnection::connect(const stx::URI& uri) {
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
#ifdef STX_ENABLE_MYSQL
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
  RAISE(kRuntimeError, "libstx was compiled without libmysqlclient");
#endif
}

std::vector<std::string> MySQLConnection::describeTable(
    const std::string& table_name) {
  std::vector<std::string> columns;

#ifdef STX_ENABLE_MYSQL
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
  RAISE(kRuntimeError, "libstx was compiled without libmysqlclient");
#endif
  return columns;
}

RefPtr<msg::MessageSchema> MySQLConnection::getTableSchema(
    const std::string& table_name) {
  std::vector<std::string> columns;
#ifdef STX_ENABLE_MYSQL
  MYSQL_RES* res = mysql_list_fields(mysql_, table_name.c_str(), NULL);
  if (res == nullptr) {
    RAISE(
      kRuntimeError,
      "mysql_list_fields() failed: %s\n",
      mysql_error(mysql_));
  }

  Vector<msg::MessageSchemaField> schema_fields;
  auto num_cols = mysql_num_fields(res);
  for (int i = 0; i < num_cols; ++i) {
    MYSQL_FIELD* col = mysql_fetch_field_direct(res, i);

    bool optional = true; // (col->flags & NOT_NULL_FLAG) > 0;
    bool is_unsigned = (col->flags & UNSIGNED_FLAG) > 0;
    msg::FieldType type;

    switch (col->type) {
      case MYSQL_TYPE_TINY:
      case MYSQL_TYPE_SHORT:
      case MYSQL_TYPE_LONG:
      case MYSQL_TYPE_INT24:
      case MYSQL_TYPE_LONGLONG:
      case MYSQL_TYPE_BIT:
        type = is_unsigned ? msg::FieldType::UINT64 : msg::FieldType::DOUBLE;
        break;

      case MYSQL_TYPE_DECIMAL:
      case MYSQL_TYPE_NEWDECIMAL:
      case MYSQL_TYPE_FLOAT:
      case MYSQL_TYPE_DOUBLE:
        type = msg::FieldType::DOUBLE;
        break;

      case MYSQL_TYPE_TIMESTAMP:
      case MYSQL_TYPE_DATE:
      case MYSQL_TYPE_TIME:
      case MYSQL_TYPE_DATETIME:
      case MYSQL_TYPE_NEWDATE:
        type = msg::FieldType::DATETIME;
        break;

      case MYSQL_TYPE_NULL:
        optional = true;
        /* fallthrough */

      case MYSQL_TYPE_STRING:
      case MYSQL_TYPE_VAR_STRING:
      case MYSQL_TYPE_BLOB:
      case MYSQL_TYPE_VARCHAR:
      case MYSQL_TYPE_TINY_BLOB:
      case MYSQL_TYPE_MEDIUM_BLOB:
      case MYSQL_TYPE_LONG_BLOB:
        type = msg::FieldType::STRING;
        break;

      case MYSQL_TYPE_SET:
      case MYSQL_TYPE_ENUM:
      case MYSQL_TYPE_GEOMETRY:
      case MYSQL_TYPE_YEAR:
#ifdef MAX_NO_FIELD_TYPES
      case MAX_NO_FIELD_TYPES:
#endif
        RAISE(kRuntimeError, "unsupported MySQL type");

    }

    schema_fields.emplace_back(
        i + 1,
        col->name,
        type,
        0,
        false,
        optional);
  }

  mysql_free_result(res);
  return new msg::MessageSchema(table_name, schema_fields);
#else
  RAISE(kRuntimeError, "libstx was compiled without libmysqlclient");
#endif
}

void MySQLConnection::executeQuery(
    const std::string& query,
    std::function<bool (const std::vector<std::string>&)> row_callback) {
#ifdef STX_ENABLE_MYSQL

#ifndef STX_NOTRACE
    stx::logTrace("fnord.mysql", "Executing MySQL query: $0", query);
#endif

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
        auto rte = dynamic_cast<const stx::Exception&>(e);
        throw rte;
      } catch (std::bad_cast bce) {
        throw e;
      }
    }
  }

  mysql_free_result(result);
#else
  RAISE(kRuntimeError, "libstx was compiled without libmysqlclient");
#endif
}

std::list<std::vector<std::string>> MySQLConnection::executeQuery(
    const std::string& query) {
  std::list<std::vector<std::string>> result_rows;
#ifdef STX_ENABLE_MYSQL

#ifndef STX_NOTRACE
    stx::logTrace("fnord.mysql", "Executing MySQL query: $0", query);
#endif

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

    result_rows.emplace_back(std::move(row_vec));
  }

  mysql_free_result(result);
#else
  RAISE(kRuntimeError, "libstx was compiled without libmysqlclient");
#endif

  return result_rows;
}

}
}
