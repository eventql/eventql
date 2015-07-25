/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORDMETRIC_MYSQLBACKEND_H
#define _FNORDMETRIC_MYSQLBACKEND_H
#include <chartsql/backends/backend.h>
#include <chartsql/backends/mysql/mysqlconnection.h>
#include <memory>
#include <mutex>
#include <vector>

namespace csql {
namespace mysql_backend {

class MySQLBackend : public csql::Backend {
public:

  static MySQLBackend* singleton();

  MySQLBackend();

  bool openTables(
      const std::vector<std::string>& table_names,
      const stx::URI& source_uri,
      std::vector<std::unique_ptr<TableRef>>* target) override;

protected:
  std::vector<std::shared_ptr<MySQLConnection>> connections_;
  std::mutex connections_mutex_;
};

}
}
#endif
