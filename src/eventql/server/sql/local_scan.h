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
#include <eventql/util/protobuf/MessageSchema.h>
#include <eventql/sql/qtree/SequentialScanNode.h>
#include <eventql/sql/runtime/compiler.h>
#include <eventql/sql/runtime/defaultruntime.h>
#include <eventql/sql/runtime/TableExpression.h>
#include <eventql/sql/runtime/ValueExpression.h>
#include <eventql/io/cstable/CSTableReader.h>
#include <eventql/db/Table.h>
#include <eventql/db/PartitionReader.h>
#include <eventql/db/partition_map.h>

//#include "eventql/eventql.h"
//
//namespace eventql {
//
//class TableScan : public csql::Task {
//public:
//
//  TableScan(
//      csql::Transaction* txn,
//      RefPtr<Table> table,
//      RefPtr<PartitionSnapshot> snap,
//      RefPtr<csql::SequentialScanNode> stmt,
//      csql::QueryBuilder* runtime,
//      csql::RowSinkFn output);
//
//  //void onInputsReady() override;
//
//  int nextRow(csql::SValue* out, int out_len) override;
//  //Option<SHA1Hash> cacheKey() const override;
//  //void setCacheKey(const SHA1Hash& key);
//
//protected:
//
//  void scanLSMTable();
//  void scanStaticTable();
//
//  csql::Transaction* txn_;
//  RefPtr<Table> table_;
//  RefPtr<PartitionSnapshot> snap_;
//  RefPtr<csql::SequentialScanNode> stmt_;
//  csql::QueryBuilder* runtime_;
//  csql::RowSinkFn output_;
//  Option<SHA1Hash> cache_key_;
//  Set<SHA1Hash> id_set_;
//};
//
//class EmptyTableScan : public csql::Task {
//public:
//  int nextRow(csql::SValue* out, int out_len) override;
//};
//
//class TableScanFactory : public csql::TaskFactory {
//public:
//
//  TableScanFactory(
//      PartitionMap* pmap,
//      String keyspace,
//      String table,
//      SHA1Hash partition,
//      RefPtr<csql::SequentialScanNode> stmt);
//
//  RefPtr<csql::Task> build(
//      csql::Transaction* txn,
//      csql::RowSinkFn output) const override;
//
//protected:
//  PartitionMap* pmap_;
//  String keyspace_;
//  String table_;
//  SHA1Hash partition_;
//  RefPtr<csql::SequentialScanNode> stmt_;
//};
//
//} // namespace eventql
