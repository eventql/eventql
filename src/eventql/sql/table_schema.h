/**
 * Copyright (c) 2016 zScale Technology GmbH <legal@zscale.io>
 * Authors:
 *   - Paul Asmuth <paul@zscale.io>
 *   - Laura Schlimmer <laura@zscale.io>
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
#pragma once
#include "eventql/eventql.h"
#include <eventql/util/stdtypes.h>
#include <eventql/util/option.h>

namespace csql {

class TableSchema {
friend class TableSchemaBuilder;
public:

  enum class ColumnClass { SCALAR, RECORD };
  enum class ColumnOptions { REPEATED, NOT_NULL };

  struct ColumnDefinition {
    ColumnClass column_class;
    String column_name;
    String column_type;
    Vector<ColumnOptions> column_options;
    Vector<ColumnDefinition const*> column_schema;
  };

  ~TableSchema();
  TableSchema(const TableSchema& other);
  TableSchema(TableSchema&& other);

  Vector<ColumnDefinition const*> getColumns() const;

protected:
  TableSchema() = default;
  Vector<ColumnDefinition*> columns_;
  Vector<ColumnDefinition const*> root_columns_;
};

using TableSchemaRef = RefPtr<TableSchema>;

class TableSchemaBuilder {
public:

  void addScalarColumn(
      const String& column_name,
      const String& column_type,
      Vector<TableSchema::ColumnOptions> column_options);

  void addRecordColumn(
      const String& column_name,
      Vector<TableSchema::ColumnOptions> column_options,
      TableSchema&& column_schema);

  TableSchema&& getTableSchema();

protected:
  TableSchema schema_;
};

} // namespace csql


