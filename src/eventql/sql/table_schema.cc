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

#include <assert.h>
#include <eventql/sql/table_schema.h>

namespace csql {

TableSchema::TableSchema(const TableSchema& other) {
  HashMap<ColumnDefinition*, ColumnDefinition*> ptr_map;
  columns_.resize(other.columns_.size());

  for (size_t i = columns_.size(); i-- > 0; ) {
    columns_[i] = new ColumnDefinition(*other.columns_[i]);
    ptr_map.emplace(other.columns_[i], columns_[i]);

    for (auto& subcol : columns_[i]->column_schema) {
      auto new_subcol = ptr_map[subcol];

      /**
       * N.B. we require the column list to be built up so that a given column
       * Ci is only ever referenced by another column Cj where j < i.
       */
      assert(new_subcol != nullptr); // invalid reference order
      subcol = new_subcol;
    }
  }

  for (auto col : other.root_columns_) {
    auto new_col = ptr_map[col];
    assert(new_col != nullptr); // invalid reference
    root_columns_.emplace_back(new_col);
  }
}

TableSchema::TableSchema(
    TableSchema&& other) :
    columns_(other.columns_),
    root_columns_(other.root_columns_) {
  other.columns_.clear();
  other.root_columns_.clear();
}

TableSchema::~TableSchema() {
  for (auto c : columns_) {
    delete c;
  }
}

TableSchema::ColumnList TableSchema::getColumns() const {
  return TableSchema::ColumnList(root_columns_.begin(), root_columns_.end());
}

TableSchema::ColumnList TableSchema::getFlatColumnList() const {
  return TableSchema::ColumnList(columns_.begin(), columns_.end());
}

void TableSchemaBuilder::addScalarColumn(
    const String& column_name,
    const String& column_type,
    Vector<TableSchema::ColumnOptions> column_options) {
  auto col_def = new TableSchema::ColumnDefinition();
  col_def->column_class = TableSchema::ColumnClass::SCALAR;
  col_def->column_name = column_name;
  col_def->full_column_name = column_name;
  col_def->column_type = column_type;
  col_def->column_options = column_options;
  schema_.columns_.emplace_back(col_def);
  schema_.root_columns_.emplace_back(col_def);
}

void TableSchemaBuilder::addRecordColumn(
    const String& column_name,
    Vector<TableSchema::ColumnOptions> column_options,
    TableSchema&& column_schema) {
  auto col_def = new TableSchema::ColumnDefinition();
  col_def->column_class = TableSchema::ColumnClass::RECORD;
  col_def->column_name = column_name;
  col_def->full_column_name = column_name;
  col_def->column_type = "RECORD";
  col_def->column_options = column_options;
  col_def->column_schema = column_schema.root_columns_;

  schema_.columns_.emplace_back(col_def);
  schema_.root_columns_.emplace_back(col_def);
  schema_.columns_.insert(
      schema_.columns_.end(),
      column_schema.columns_.begin(),
      column_schema.columns_.end());

  for (auto subcol : column_schema.columns_) {
    auto mutable_subcol = const_cast<TableSchema::ColumnDefinition*>(subcol);

    mutable_subcol->full_column_name = StringUtil::format(
        "$0.$1",
        column_name,
        subcol->full_column_name);
  }

  column_schema.columns_.clear();
  column_schema.root_columns_.clear();
}

TableSchema&& TableSchemaBuilder::getTableSchema() {
  return std::move(schema_);
}

} // namespace csql



