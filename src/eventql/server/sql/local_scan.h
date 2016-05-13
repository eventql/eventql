/**
 * Copyright (c) 2015 - zScale Technology GmbH <legal@zscale.io>
 *   All Rights Reserved.
 *
 * Authors:
 *   Paul Asmuth <paul@zscale.io>
 *
 * This file is CONFIDENTIAL -- Distribution or duplication of this material or
 * the information contained herein is strictly forbidden unless prior written
 * permission is obtained.
 */
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/protobuf/MessageSchema.h>
#include <eventql/sql/qtree/SequentialScanNode.h>
#include <eventql/sql/runtime/compiler.h>
#include <eventql/sql/runtime/defaultruntime.h>
#include <eventql/sql/runtime/TableExpression.h>
#include <eventql/sql/runtime/ValueExpression.h>
#include <eventql/infra/cstable/CSTableReader.h>
#include <zbase/core/Table.h>
#include <zbase/core/PartitionReader.h>
#include <zbase/core/PartitionMap.h>

//using namespace stx;
//
//namespace zbase {
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
//} // namespace zbase
