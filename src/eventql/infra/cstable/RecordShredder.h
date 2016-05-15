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
      const util::json::JSONObject::const_iterator& begin,
      const util::json::JSONObject::const_iterator& end);

  void addRecordFromProtobuf(const util::msg::DynamicMessage& msg);
  void addRecordFromProtobuf(
      const util::msg::MessageObject& msg,
      const util::msg::MessageSchema& schema);


  void addRecordsFromCSV(util::CSVInputStream* csv);

protected:
  CSTableWriter* writer_;
  const TableSchema* schema_;
};

} // namespace cstable


