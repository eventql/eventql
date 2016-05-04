/**
 * This file is part of the "libcstable" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * libstx is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <cstable/cstable.h>
#include <cstable/TableSchema.h>

using namespace stx;

namespace cstable {

TableSchema::TableSchema() {}

TableSchema::TableSchema(const TableSchema& other) {
  for (const auto& c : other.columns_by_name_) {
    auto col_cpy = mkScoped(new Column());
    col_cpy->name = c.second->name;
    col_cpy->type = c.second->type;
    col_cpy->encoding = c.second->encoding;
    col_cpy->type_size = c.second->type_size;
    col_cpy->repeated = c.second->repeated;
    col_cpy->optional = c.second->optional;
    if (c.second->subschema.get()) {
      col_cpy->subschema = mkScoped(new TableSchema(*c.second->subschema));
    }

    columns_.emplace_back(col_cpy.get());
    columns_by_name_.emplace(col_cpy->name, std::move(col_cpy));
  }
}

void TableSchema::addBool(
    const String& name,
    bool optional /* = true */,
    ColumnEncoding encoding /* = ColumnEncoding::BOOLEAN_BITPACKED */) {
  auto col = mkScoped(new Column());
  col->name = name;
  col->type = ColumnType::BOOLEAN;
  col->encoding = encoding;
  col->type_size = 0;
  col->repeated = false;
  col->optional = optional;

  columns_.emplace_back(col.get());
  columns_by_name_.emplace(name, std::move(col));
}

void TableSchema::addBoolArray(
    const String& name,
    bool optional /* = true */,
    ColumnEncoding encoding /* = ColumnEncoding::BOOLEAN_BITPACKED */) {
  auto col = mkScoped(new Column());
  col->name = name;
  col->type = ColumnType::BOOLEAN;
  col->encoding = encoding;
  col->type_size = 0;
  col->repeated = true;
  col->optional = optional;

  columns_.emplace_back(col.get());
  columns_by_name_.emplace(name, std::move(col));
}

void TableSchema::addUnsignedInteger(
    const String& name,
    bool optional /* = true */,
    ColumnEncoding encoding /* = ColumnEncoding::UINT64_LEB128 */,
    uint64_t max_value /* = 0 */) {
  auto col = mkScoped(new Column());
  col->name = name;
  col->type = ColumnType::UNSIGNED_INT;
  col->encoding = encoding;
  col->type_size = max_value;
  col->repeated = false;
  col->optional = optional;

  columns_.emplace_back(col.get());
  columns_by_name_.emplace(name, std::move(col));
}

void TableSchema::addUnsignedIntegerArray(
    const String& name,
    bool optional /* = true */,
    ColumnEncoding encoding /* = ColumnEncoding::UINT64_LEB128 */,
    uint64_t max_value /* = 0 */) {
  auto col = mkScoped(new Column());
  col->name = name;
  col->type = ColumnType::UNSIGNED_INT;
  col->encoding = encoding;
  col->type_size = max_value;
  col->repeated = true;
  col->optional = optional;

  columns_.emplace_back(col.get());
  columns_by_name_.emplace(name, std::move(col));
}

void TableSchema::addFloat(
    const String& name,
    bool optional /* = true */,
    ColumnEncoding encoding /* = ColumnEncoding::FLOAT_IEEE754 */) {
  auto col = mkScoped(new Column());
  col->name = name;
  col->type = ColumnType::FLOAT;
  col->encoding = encoding;
  col->type_size = 0;
  col->repeated = false;
  col->optional = optional;

  columns_.emplace_back(col.get());
  columns_by_name_.emplace(name, std::move(col));
}

void TableSchema::addFloatArray(
    const String& name,
    bool optional /* = true */,
    ColumnEncoding encoding /* = ColumnEncoding::FLOAT_IEEE754 */) {
  auto col = mkScoped(new Column());
  col->name = name;
  col->type = ColumnType::FLOAT;
  col->encoding = encoding;
  col->type_size = 0;
  col->repeated = true;
  col->optional = optional;

  columns_.emplace_back(col.get());
  columns_by_name_.emplace(name, std::move(col));
}

void TableSchema::addString(
    const String& name,
    bool optional /* = true */,
    ColumnEncoding encoding /* = ColumnEncoding::STRING_PLAIN */,
    uint64_t max_len /* = 0 */) {
  auto col = mkScoped(new Column());
  col->name = name;
  col->type = ColumnType::STRING;
  col->encoding = encoding;
  col->type_size = max_len;
  col->repeated = false;
  col->optional = optional;

  columns_.emplace_back(col.get());
  columns_by_name_.emplace(name, std::move(col));
}

void TableSchema::addStringArray(
    const String& name,
    bool optional /* = true */,
    ColumnEncoding encoding /* = ColumnEncoding::STRING_PLAIN */,
    uint64_t max_len /* = 0 */) {
  auto col = mkScoped(new Column());
  col->name = name;
  col->type = ColumnType::STRING;
  col->encoding = encoding;
  col->type_size = max_len;
  col->repeated = true;
  col->optional = optional;

  columns_.emplace_back(col.get());
  columns_by_name_.emplace(name, std::move(col));
}

void TableSchema::addSubrecord(
    const String& name,
    TableSchema schema,
    bool optional /* = true */) {
  auto col = mkScoped(new Column());
  col->name = name;
  col->type = ColumnType::SUBRECORD;
  col->repeated = false;
  col->optional = optional;
  col->subschema = mkScoped(new TableSchema(schema));

  columns_.emplace_back(col.get());
  columns_by_name_.emplace(name, std::move(col));
}

void TableSchema::addSubrecordArray(
    const String& name,
    TableSchema schema,
    bool optional /* = true */) {
  auto col = mkScoped(new Column());
  col->name = name;
  col->type = ColumnType::SUBRECORD;
  col->repeated = true;
  col->optional = optional;
  col->subschema = mkScoped(new TableSchema(schema));

  columns_.emplace_back(col.get());
  columns_by_name_.emplace(name, std::move(col));
}

void TableSchema::addColumn(
    const String& name,
    ColumnType type,
    ColumnEncoding encoding,
    bool repeated,
    bool optional,
    uint64_t type_size /* = 0 */) {
  auto col = mkScoped(new Column());
  col->name = name;
  col->type = type;
  col->encoding = encoding;
  col->repeated = repeated;
  col->optional = optional;
  col->type_size = type_size;

  columns_.emplace_back(col.get());
  columns_by_name_.emplace(name, std::move(col));
}

const Vector<TableSchema::Column*>& TableSchema::columns() const {
  return columns_;
}

TableSchema TableSchema::fromProtobuf(const msg::MessageSchema& schema) {
  TableSchema rs;

  for (const auto& f : schema.fields()) {
    switch (f.type) {

      case msg::FieldType::OBJECT:
        if (f.repeated) {
          rs.addSubrecordArray(
              f.name,
              TableSchema::fromProtobuf(*f.schema),
              f.optional);
        } else {
          rs.addSubrecord(
              f.name,
              TableSchema::fromProtobuf(*f.schema),
              f.optional);
        }
        break;

      case msg::FieldType::BOOLEAN:
        rs.addColumn(
            f.name,
            ColumnType::BOOLEAN,
            ColumnEncoding::BOOLEAN_BITPACKED,
            f.repeated,
            f.optional);
        break;

      case msg::FieldType::STRING:
        rs.addColumn(
            f.name,
            ColumnType::STRING,
            ColumnEncoding::STRING_PLAIN,
            f.repeated,
            f.optional);
        break;

      case msg::FieldType::UINT64:
      case msg::FieldType::UINT32:
        rs.addColumn(
            f.name,
            ColumnType::UNSIGNED_INT,
            ColumnEncoding::UINT64_LEB128,
            f.repeated,
            f.optional);
        break;

      case msg::FieldType::DOUBLE:
        rs.addColumn(
            f.name,
            ColumnType::FLOAT,
            ColumnEncoding::FLOAT_IEEE754,
            f.repeated,
            f.optional);
        break;

      case msg::FieldType::DATETIME:
        rs.addColumn(
            f.name,
            ColumnType::DATETIME,
            ColumnEncoding::UINT64_LEB128,
            f.repeated,
            f.optional);
        break;

    }
  }

  return rs;
}

static void listFlatColumns(
    const String& prefix,
    uint32_t r_max,
    uint32_t d_max,
    const TableSchema::Column* field,
    Vector<ColumnConfig>* columns) {
  auto colname = prefix + field->name;

  if (field->repeated) {
    ++r_max;
  }

  if (field->repeated || field->optional) {
    ++d_max;
  }

  if (field->type == ColumnType::SUBRECORD) {
    for (const auto& f : field->subschema->columns()) {
      listFlatColumns(colname + ".", r_max, d_max, f, columns);
    }
  } else {
    cstable::ColumnConfig cconf;
    cconf.column_name = colname;
    cconf.storage_type = field->encoding;
    cconf.logical_type = field->type;
    cconf.rlevel_max = r_max;
    cconf.dlevel_max = d_max;
    columns->emplace_back(cconf);
  }
}

Vector<ColumnConfig> TableSchema::flatColumns() const {
  Vector<ColumnConfig> columns;

  for (const auto& f : columns_) {
    listFlatColumns("", 0, 0, f, &columns);
  }

  return columns;
}


} // namespace msg


