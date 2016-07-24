/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#include <eventql/sql/backends/mysql/mysqlbackend.h>
#include <eventql/sql/backends/mysql/mysqlconnection.h>
#include <eventql/sql/backends/mysql/mysqltableref.h>
#include <eventql/util/exception.h>
#include <memory>
#include <mutex>

namespace csql {
namespace mysql_backend {

MySQLBackend* MySQLBackend::singleton() {
  static MySQLBackend singleton_backend;
  return &singleton_backend;
}

static std::mutex global_mysql_init_lock;
static bool global_mysql_initialized = false;

MySQLBackend::MySQLBackend() {
#ifdef FNORD_ENABLE_MYSQL
  global_mysql_init_lock.lock();
  if (!global_mysql_initialized) {
    mysql_library_init(0, NULL, NULL); // FIXPAUl mysql_library_end();
    global_mysql_initialized = true;
  }
  global_mysql_init_lock.unlock();
#endif
}

bool MySQLBackend::openTables(
    const std::vector<std::string>& table_names,
    const URI& source_uri,
    std::vector<std::unique_ptr<TableRef>>* target) {
  if (source_uri.scheme() != "mysql") {
    return false;
  }

  // FIXPAUL move all of this into a mysql thread/connection pool
  std::shared_ptr<MySQLConnection> conn =
      MySQLConnection::openConnection(source_uri);

  for (const auto& tbl : table_names) {
    target->emplace_back(new MySQLTableRef(conn, tbl));
  }

  connections_mutex_.lock();
  connections_.push_back(std::move(conn));
  connections_mutex_.unlock();

  return true;
}

}
}
