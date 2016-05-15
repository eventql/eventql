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
#ifndef _FNORDMETRIC_CSVBACKEND_H
#define _FNORDMETRIC_CSVBACKEND_H
#include <memory>
#include <eventql/sql/backends/backend.h>
#include <eventql/sql/backends/csv/csvtableref.h>

namespace csql {
class ASTNode;
namespace csv_backend {

class CSVBackend : public csql::Backend {
public:

  static CSVBackend* singleton();

  bool openTables(
      const std::vector<std::string>& table_names,
      const stx::URI& source_uri,
      std::vector<std::unique_ptr<TableRef>>* target) override;

};

}
}
#endif
