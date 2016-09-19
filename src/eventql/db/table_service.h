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
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/random.h>
#include <eventql/util/option.h>
#include <eventql/util/protobuf/DynamicMessage.h>
#include <eventql/util/thread/queue.h>
#include <eventql/util/thread/eventloop.h>
#include <eventql/util/mdb/MDB.h>
#include <eventql/db/table_config.pb.h>
#include <eventql/db/partition.h>
#include <eventql/db/table_info.h>
#include <eventql/db/partition_info.pb.h>
#include <eventql/db/record_envelope.pb.h>
#include <eventql/db/partition_map.h>
#include <eventql/db/shredded_record.h>
#include <eventql/config/config_directory.h>

#include "eventql/eventql.h"

namespace eventql {

enum class InsertFlags : uint64_t {
  REPLICATED_WRITE = 1,
  SYNC_COMMIT = 2
};

class TableService {
public:

  enum AlterTableOperationType {
    OP_ADD_COLUMN,
    OP_REMOVE_COLUMN
  };

  struct AlterTableOperation {
    AlterTableOperationType optype;
    String field_name;
    msg::FieldType field_type;
    bool is_repeated;
    bool is_optional;
  };

  TableService(
      ConfigDirectory* cdir,
      PartitionMap* pmap,
      ProcessConfig* config);

  Status createTable(
      const String& db_namespace,
      const String& table_name,
      const msg::MessageSchema& schema,
      Vector<String> primary_key,
      const std::vector<std::pair<std::string, std::string>>& properties);

  Status alterTable(
      const String& db_namespace,
      const String& table_name,
      Vector<AlterTableOperation> operations);

  Status dropTable(
      const String& db_namespace,
      const String& table_name);

  void listTables(
      const String& tsdb_namespace,
      Function<void (const TableDefinition& table)> fn) const;

  // insert one record
  ReturnCode insertRecord(
      const String& tsdb_namespace,
      const String& table_name,
      const msg::DynamicMessage& data,
      uint64_t flags = 0);

  // insert a batch of records
  ReturnCode insertRecords(
      const String& tsdb_namespace,
      const String& table_name,
      const msg::DynamicMessage* begin,
      const msg::DynamicMessage* end,
      uint64_t flags = 0);

  // insert a single record from json
  ReturnCode insertRecord(
      const String& tsdb_namespace,
      const String& table_name,
      const json::JSONObject::const_iterator& data_begin,
      const json::JSONObject::const_iterator& data_end,
      uint64_t flags = 0);

  // internal method, don't use
  ReturnCode insertReplicatedRecords(
      const String& tsdb_namespace,
      const String& table_name,
      const SHA1Hash& partition_key,
      const ShreddedRecordList& records);

  void compactPartition(
      const String& tsdb_namespace,
      const String& table_name,
      const SHA1Hash& partition_key);

  void commitPartition(
      const String& tsdb_namespace,
      const String& table_name,
      const SHA1Hash& partition_key);

  Option<RefPtr<msg::MessageSchema>> tableSchema(
      const String& tsdb_namespace,
      const String& table_key);

  Option<TableDefinition> tableConfig(
      const String& tsdb_namespace,
      const String& table_key);

protected:

  ReturnCode insertRecords(
      const String& tsdb_namespace,
      const String& table_name,
      const SHA1Hash& partition_key,
      const Set<String>& servers,
      const ShreddedRecordList& records);

  ReturnCode insertRecordsLocal(
      const String& tsdb_namespace,
      const String& table_name,
      const SHA1Hash& partition_key,
      const ShreddedRecordList& records);

  ConfigDirectory* cdir_;
  PartitionMap* pmap_;
  ProcessConfig* config_;
};

} // namespace tdsb

