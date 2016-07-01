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
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/exception.h>
#include <eventql/util/autoref.h>
#include <eventql/util/json/json.h>
#include <eventql/util/protobuf/MessageSchema.h>

namespace cstable {

class TableSchema : public RefCounted {
public:

  struct Column {
    String name;
    ColumnType type;
    ColumnEncoding encoding;
    uint64_t type_size;
    uint64_t column_id;
    bool repeated;
    bool optional;
    ScopedPtr<TableSchema> subschema;
  };

  TableSchema();
  TableSchema(const TableSchema& other);

  void addBool(
      const String& name,
      bool optional = true,
      ColumnEncoding encoding = ColumnEncoding::BOOLEAN_BITPACKED);

  void addBoolArray(
      const String& name,
      bool optional = true,
      ColumnEncoding encoding = ColumnEncoding::BOOLEAN_BITPACKED);

  void addUnsignedInteger(
      const String& name,
      bool optional = true,
      ColumnEncoding encoding = ColumnEncoding::UINT64_LEB128,
      uint64_t max_value = 0);

  void addUnsignedIntegerArray(
      const String& name,
      bool optional = true,
      ColumnEncoding encoding = ColumnEncoding::UINT64_LEB128,
      uint64_t max_value = 0);

  void addSignedInteger(
      const String& name,
      bool optional = true,
      ColumnEncoding encoding = ColumnEncoding::UINT64_LEB128,
      uint64_t max_value = 0);

  void addSignedIntegerArray(
      const String& name,
      bool optional = true,
      ColumnEncoding encoding = ColumnEncoding::UINT64_LEB128,
      uint64_t max_value = 0);

  void addFloat(
      const String& name,
      bool optional = true,
      ColumnEncoding encoding = ColumnEncoding::FLOAT_IEEE754);

  void addFloatArray(
      const String& name,
      bool optional = true,
      ColumnEncoding encoding = ColumnEncoding::FLOAT_IEEE754);

  void addString(
      const String& name,
      bool optional = true,
      ColumnEncoding encoding = ColumnEncoding::STRING_PLAIN,
      uint64_t max_length = 0);

  void addStringArray(
      const String& name,
      bool optional = true,
      ColumnEncoding encoding = ColumnEncoding::STRING_PLAIN,
      uint64_t max_length = 0);

  void addSubrecord(
      const String& name,
      TableSchema subschema,
      bool optional = true);

  void addSubrecordArray(
      const String& name,
      TableSchema subschema,
      bool optional = true);

  void addColumn(
      const String& name,
      ColumnType type,
      ColumnEncoding encoding,
      bool repeated,
      bool optional,
      uint64_t type_size = 0);

  const Vector<Column*>& columns() const;

  //void toJSON(json::JSONOutputStream* json) const;
  //void fromJSON(
  //    json::JSONObject::const_iterator begin,
  //    json::JSONObject::const_iterator end);

  static TableSchema fromProtobuf(const msg::MessageSchema& schema);

  Vector<ColumnConfig> flatColumns() const;

protected:
  HashMap<String, ScopedPtr<Column>> columns_by_name_;
  Vector<Column*> columns_;
  size_t next_column_id_;
};

} // namespace cstable

