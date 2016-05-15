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
#include <memory>
#include <eventql/util/exception.h>
#include <eventql/util/io/inputstream.h>
#include <eventql/sql/backends/csv/csvbackend.h>
#include <eventql/sql/backends/csv/csvtableref.h>
#include <eventql/sql/parser/astnode.h>
#include <eventql/sql/parser/token.h>

namespace csql {
namespace csv_backend {

CSVBackend* CSVBackend::singleton() {
  static CSVBackend singleton_backend;
  return &singleton_backend;
}

bool CSVBackend::openTables(
    const std::vector<std::string>& table_names,
    const stx::URI& source_uri,
    std::vector<std::unique_ptr<TableRef>>* target) {
  if (source_uri.scheme() != "csv") {
    return false;
  }

  if (table_names.size() != 1) {
    RAISE(
        kRuntimeError,
        "CSVBackend can only import exactly one table per source");
  }

  char col_sep = ',';
  char row_sep = '\n';
  char quote_char = '"';
  char escape_char = '\\';
  bool headers = false;

  for (const auto& param : source_uri.queryParams()) {
    if (param.first == "headers") {
      headers = param.second == "true" || param.second == "TRUE";
      continue;
    }

    if (param.first == "row_sep") {
      if (param.second.size() != 1) {
        RAISE(
            kRuntimeError,
            "invalid parameter %s for CSVBackend: '%s', must be a single "
                "character",
            param.first.c_str(),
            param.second.c_str());
      }

      row_sep = param.second[0];
      continue;
    }

    if (param.first == "col_sep") {
      if (param.second.size() != 1) {
        RAISE(
            kRuntimeError,
            "invalid parameter %s for CSVBackend: '%s', must be a single "
                "character",
            param.first.c_str(),
            param.second.c_str());
      }

      col_sep = param.second[0];
      continue;
    }

    if (param.first == "quote_char") {
      if (param.second.size() != 1) {
        RAISE(
            kRuntimeError,
            "invalid parameter %s for CSVBackend: '%s', must be a single "
                "character",
            param.first.c_str(),
            param.second.c_str());
      }

      quote_char = param.second[0];
      continue;
    }

    if (param.first == "escape_char") {
      if (param.second.size() != 1) {
        RAISE(
            kRuntimeError,
            "invalid parameter %s for CSVBackend: '%s', must be a single "
                "character",
            param.first.c_str(),
            param.second.c_str());
      }

      escape_char = param.second[0];
      continue;
    }

    RAISE(
        kRuntimeError,
        "invalid parameter for CSVBackend: '%s'", param.first.c_str());
  }

  auto csv = CSVInputStream::openFile(
      source_uri.path(),
      col_sep,
      row_sep,
      quote_char);

  target->emplace_back(
      std::unique_ptr<TableRef>(new CSVTableRef(std::move(csv), headers)));

  return true;
}

/*$
std::unique_ptr<TableRef> CSVBackend::openTable(ASTNode* import) {
}

*/

}
}
