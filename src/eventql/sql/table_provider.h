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
#include <eventql/util/option.h>
#include <eventql/util/status.h>
#include <eventql/sql/svalue.h>
#include <eventql/sql/table_schema.h>
#include <eventql/sql/TableInfo.h>
#include <eventql/sql/expressions/table_expression.h>
#include <eventql/sql/qtree/SequentialScanNode.h>
#include <eventql/sql/scheduler/execution_context.h>
#include "eventql/sql/qtree/nodes/alter_table.h"
#include "eventql/sql/qtree/nodes/create_table.h"
#include "eventql/db/table_info.h"
#include "eventql/config/config_directory.h"

namespace csql {

class Transaction;

class TableProvider : public RefCounted {
public:

  virtual Option<ScopedPtr<TableExpression>> buildSequentialScan(
      Transaction* ctx,
      ExecutionContext* execution_context,
      RefPtr<SequentialScanNode> seqscan) const = 0;

  virtual void listTables(Function<void (const TableInfo& table)> fn) const = 0;

  Status listPartitions(
      const String& table_name,
      Function<void (const ::eventql::TablePartitionInfo& partition)> fn) const {
    return Status(eRuntimeError, "not yet implemented");
  }

  Status listServers(
      Function<void (const ::eventql::ServerConfig& server)> fn) const {
    return Status(eNotImplementedError, "not implemented");
  }

  virtual Option<TableInfo> describe(const String& table_name) const = 0;

  virtual Status createTable(const CreateTableNode& create_table) {
    RAISE(kRuntimeError, "can't create tables");
  }

  virtual Status createDatabase(const std::string& database_name) {
    RAISE(kRuntimeError, "can't create databases");
  }

  virtual Status dropTable(const String& table_name) {
    RAISE(kRuntimeError, "can't drop table");
  }

  virtual Status alterTable(const AlterTableNode& alter_table) {
    RAISE(kRuntimeError, "can't alter tables");
  }

  virtual Status insertRecord(
      const String& table_name,
      Vector<Pair<String, SValue>> data) {
    RAISE(kRuntimeError, "can't insert");
  }

  virtual Status insertRecord(
      const String& table_name,
      const String& json_str) {
    RAISE(kRuntimeError, "can't insert");
  }
};

}
