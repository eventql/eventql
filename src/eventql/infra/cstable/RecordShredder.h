/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/io/file.h>
#include <eventql/util/util/binarymessagewriter.h>
#include <eventql/util/autoref.h>
#include <eventql/util/csv/CSVInputStream.h>
#include <eventql/infra/cstable/ColumnWriter.h>
#include <eventql/infra/cstable/CSTableWriter.h>
#include <eventql/infra/cstable/TableSchema.h>
#include <eventql/util/protobuf/MessageSchema.h>
#include <eventql/util/protobuf/DynamicMessage.h>


namespace cstable {

class RecordShredder {
public:

  RecordShredder(
      CSTableWriter* writer);

  RecordShredder(
      CSTableWriter* writer,
      const TableSchema* schema);

  void addRecordFromJSON(const String& json);
  void addRecordFromJSON(
      const stx::json::JSONObject::const_iterator& begin,
      const stx::json::JSONObject::const_iterator& end);

  void addRecordFromProtobuf(const stx::msg::DynamicMessage& msg);
  void addRecordFromProtobuf(
      const stx::msg::MessageObject& msg,
      const stx::msg::MessageSchema& schema);


  void addRecordsFromCSV(stx::CSVInputStream* csv);

protected:
  CSTableWriter* writer_;
  const TableSchema* schema_;
};

} // namespace cstable


