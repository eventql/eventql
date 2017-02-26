/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
 *   - Laura Schlimmer <laura@eventql.io>
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
#include "eventql/eventql.h"
#include <eventql/util/stdtypes.h>
#include <eventql/util/exception.h>
#include <eventql/util/wallclock.h>
#include "eventql/sql/svalue.h"
#include "eventql/sql/runtime/defaultruntime.h"
#include "eventql/sql/qtree/SequentialScanNode.h"
#include "eventql/sql/qtree/ColumnReferenceNode.h"
#include "eventql/sql/qtree/CallExpressionNode.h"
#include "eventql/sql/qtree/LiteralExpressionNode.h"
#include "eventql/sql/qtree/QueryTreeUtil.h"
#include "eventql/sql/qtree/qtree_coder.h"
#include "eventql/sql/qtree/nodes/create_database.h"
#include "eventql/sql/qtree/nodes/use_database.h"
#include "eventql/sql/qtree/nodes/alter_table.h"
#include "eventql/sql/qtree/nodes/create_table.h"
#include "eventql/sql/qtree/nodes/drop_table.h"
#include "eventql/sql/qtree/nodes/insert_into.h"
#include "eventql/sql/qtree/nodes/insert_json.h"
#include "eventql/sql/qtree/nodes/describe_partitions.h"
#include "eventql/sql/qtree/nodes/cluster_show_servers.h"
#include "eventql/sql/CSTableScanProvider.h"
#include "eventql/sql/drivers/csv/CSVTableProvider.h"
#include "../unit_test.h"
#include "../test_runner.h"

namespace eventql {
namespace test {
namespace unit {

using namespace csql;

// UNIT-QTREE-001
bool test_sql_qtree_ExtractEqualsConstraint(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  txn->setTableProvider(
      new CSTableScanProvider(
          "testtable",
          FileUtil::joinPaths(ctx->srcdir, "test/sql_testdata/testtbl.cst")));

  Vector<String> queries;
  queries.push_back("select 1 from testtable where time = 1234;");
  queries.push_back("select 1 from testtable where 1234 = time;");
  queries.push_back("select 1 from testtable where time = 1000 + 234;");
  queries.push_back("select 1 from testtable where 1000 + 234 = time;");
  for (const auto& query : queries) {
    csql::Parser parser;
    parser.parse(query.data(), query.size());

    auto qtree_builder = runtime->queryPlanBuilder();
    auto qtrees = qtree_builder->build(
        txn.get(),
        parser.getStatements(),
        txn->getTableProvider());

    EXPECT(!qtrees.empty());
    auto qtree = qtrees[0];
    EXPECT_TRUE(dynamic_cast<SequentialScanNode*>(qtree.get()) != nullptr);
    auto seqscan = qtree.asInstanceOf<SequentialScanNode>();
    EXPECT_EQ(seqscan->tableName(), "testtable");

    auto constraints = seqscan->constraints();
    EXPECT_EQ(constraints.size(), 1u);

    auto constraint = constraints[0];
    EXPECT_EQ(constraint.column_name, "time");
    EXPECT_TRUE(constraint.type == ScanConstraintType::EQUAL_TO);
    EXPECT_EQ(constraint.value.toString(), "1234");
  }
  return true;
}

// UNIT-QTREE-002
bool test_sql_qtree_ExtractNotEqualsConstraint(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  txn->setTableProvider(
      new CSTableScanProvider(
          "testtable",
          FileUtil::joinPaths(ctx->srcdir, "test/sql_testdata/testtbl.cst")));

  Vector<String> queries;
  queries.push_back("select 1 from testtable where time != 1234;");
  queries.push_back("select 1 from testtable where time != 1000 + 234;");
  queries.push_back("select 1 from testtable where 1234 != time;");
  queries.push_back("select 1 from testtable where 1000 + 234 != time;");
  for (const auto& query : queries) {
    csql::Parser parser;
    parser.parse(query.data(), query.size());

    auto qtree_builder = runtime->queryPlanBuilder();
    auto qtrees = qtree_builder->build(
        txn.get(),
        parser.getStatements(),
        txn->getTableProvider());

    EXPECT_EQ(qtrees.size(), 1u);
    auto qtree = qtrees[0];
    EXPECT_TRUE(dynamic_cast<SequentialScanNode*>(qtree.get()) != nullptr);
    auto seqscan = qtree.asInstanceOf<SequentialScanNode>();
    EXPECT_EQ(seqscan->tableName(), "testtable");

    auto constraints = seqscan->constraints();
    EXPECT_EQ(constraints.size(), 1u);

    auto constraint = constraints[0];
    EXPECT_EQ(constraint.column_name, "time");
    EXPECT_TRUE(constraint.type == ScanConstraintType::NOT_EQUAL_TO);
    EXPECT_EQ(constraint.value.toString(), "1234");
  }
  return true;
}

// UNIT-QTREE-003
bool test_sql_qtree_ExtractLessThanConstraint(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  txn->setTableProvider(
      new CSTableScanProvider(
          "testtable",
          FileUtil::joinPaths(ctx->srcdir, "test/sql_testdata/testtbl.cst")));

  Vector<String> queries;
  queries.push_back("select 1 from testtable where time < 1234;");
  queries.push_back("select 1 from testtable where time < 1000 + 234;");
  queries.push_back("select 1 from testtable where 1234 > time;");
  queries.push_back("select 1 from testtable where 1000 + 234 > time;");
  for (const auto& query : queries) {
    csql::Parser parser;
    parser.parse(query.data(), query.size());

    auto qtree_builder = runtime->queryPlanBuilder();
    auto qtrees = qtree_builder->build(
        txn.get(),
        parser.getStatements(),
        txn->getTableProvider());

    EXPECT_EQ(qtrees.size(), 1u);
    auto qtree = qtrees[0];
    EXPECT_TRUE(dynamic_cast<SequentialScanNode*>(qtree.get()) != nullptr);
    auto seqscan = qtree.asInstanceOf<SequentialScanNode>();
    EXPECT_EQ(seqscan->tableName(), "testtable");

    auto constraints = seqscan->constraints();
    EXPECT_EQ(constraints.size(), 1u);

    auto constraint = constraints[0];
    EXPECT_EQ(constraint.column_name, "time");
    EXPECT_TRUE(constraint.type == ScanConstraintType::LESS_THAN);
    EXPECT_EQ(constraint.value.toString(), "1234");
  }
  return true;
}

// UNIT-QTREE-004
bool test_sql_qtree_ExtractLessThanOrEqualToConstraint(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  txn->setTableProvider(
      new CSTableScanProvider(
          "testtable",
          FileUtil::joinPaths(ctx->srcdir, "test/sql_testdata/testtbl.cst")));

  Vector<String> queries;
  queries.push_back("select 1 from testtable where time <= 1234;");
  queries.push_back("select 1 from testtable where time <= 1000 + 234;");
  queries.push_back("select 1 from testtable where 1234 >= time;");
  queries.push_back("select 1 from testtable where 1000 + 234 >= time;");
  for (const auto& query : queries) {
    csql::Parser parser;
    parser.parse(query.data(), query.size());

    auto qtree_builder = runtime->queryPlanBuilder();
    auto qtrees = qtree_builder->build(
        txn.get(),
        parser.getStatements(),
        txn->getTableProvider());

    EXPECT_EQ(qtrees.size(), 1u);
    auto qtree = qtrees[0];
    EXPECT_TRUE(dynamic_cast<SequentialScanNode*>(qtree.get()) != nullptr);
    auto seqscan = qtree.asInstanceOf<SequentialScanNode>();
    EXPECT_EQ(seqscan->tableName(), "testtable");

    auto constraints = seqscan->constraints();
    EXPECT_EQ(constraints.size(), 1u);

    auto constraint = constraints[0];
    EXPECT_EQ(constraint.column_name, "time");
    EXPECT_TRUE(constraint.type == ScanConstraintType::LESS_THAN_OR_EQUAL_TO);
    EXPECT_EQ(constraint.value.toString(), "1234");
  }
  return true;
}

// UNIT-QTREE-005
bool test_sql_qtree_ExtractGreaterThanConstraint(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  txn->setTableProvider(
      new CSTableScanProvider(
          "testtable",
          FileUtil::joinPaths(ctx->srcdir, "test/sql_testdata/testtbl.cst")));

  Vector<String> queries;
  queries.push_back("select 1 from testtable where time > 1234;");
  queries.push_back("select 1 from testtable where time > 1000 + 234;");
  queries.push_back("select 1 from testtable where 1234 < time;");
  queries.push_back("select 1 from testtable where 1000 + 234 < time;");
  for (const auto& query : queries) {
    csql::Parser parser;
    parser.parse(query.data(), query.size());

    auto qtree_builder = runtime->queryPlanBuilder();
    auto qtrees = qtree_builder->build(
        txn.get(),
        parser.getStatements(),
        txn->getTableProvider());

    EXPECT_EQ(qtrees.size(), 1u);
    auto qtree = qtrees[0];
    EXPECT_TRUE(dynamic_cast<SequentialScanNode*>(qtree.get()) != nullptr);
    auto seqscan = qtree.asInstanceOf<SequentialScanNode>();
    EXPECT_EQ(seqscan->tableName(), "testtable");

    auto constraints = seqscan->constraints();
    EXPECT_EQ(constraints.size(), 1u);

    auto constraint = constraints[0];
    EXPECT_EQ(constraint.column_name, "time");
    EXPECT_TRUE(constraint.type == ScanConstraintType::GREATER_THAN);
    EXPECT_EQ(constraint.value.toString(), "1234");
  }
  return true;
}

// UNIT-QTREE-006
bool test_sql_qtree_ExtractGreaterThanOrEqualToConstraint(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  txn->setTableProvider(
      new CSTableScanProvider(
          "testtable",
          FileUtil::joinPaths(ctx->srcdir, "test/sql_testdata/testtbl.cst")));

  Vector<String> queries;
  queries.push_back("select 1 from testtable where time >= 1234;");
  queries.push_back("select 1 from testtable where time >= 1000 + 234;");
  queries.push_back("select 1 from testtable where 1234 <= time;");
  queries.push_back("select 1 from testtable where 1000 + 234 <= time;");
  for (const auto& query : queries) {
    csql::Parser parser;
    parser.parse(query.data(), query.size());

    auto qtree_builder = runtime->queryPlanBuilder();
    auto qtrees = qtree_builder->build(
        txn.get(),
        parser.getStatements(),
        txn->getTableProvider());

    EXPECT_EQ(qtrees.size(), 1u);
    auto qtree = qtrees[0];
    EXPECT_TRUE(dynamic_cast<SequentialScanNode*>(qtree.get()) != nullptr);
    auto seqscan = qtree.asInstanceOf<SequentialScanNode>();
    EXPECT_EQ(seqscan->tableName(), "testtable");

    auto constraints = seqscan->constraints();
    EXPECT_EQ(constraints.size(), 1u);

    auto constraint = constraints[0];
    EXPECT_EQ(constraint.column_name, "time");
    EXPECT_TRUE(constraint.type == ScanConstraintType::GREATER_THAN_OR_EQUAL_TO);
    EXPECT_EQ(constraint.value.toString(), "1234");
  }
  return true;
}

// UNIT-QTREE-007
bool test_sql_qtree_ExtractMultipleConstraints(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  txn->setTableProvider(
      new CSTableScanProvider(
          "testtable",
          FileUtil::joinPaths(ctx->srcdir, "test/sql_testdata/testtbl.cst")));

  String query = "select 1 from testtable where 1000 + 200 + 30 + 4 > time AND session_id != 'blah' AND time >= 1111 * 6;";

  csql::Parser parser;
  parser.parse(query.data(), query.size());

  auto qtree_builder = runtime->queryPlanBuilder();
  auto qtrees = qtree_builder->build(
      txn.get(),
      parser.getStatements(),
      txn->getTableProvider());

  EXPECT_EQ(qtrees.size(), 1u);
  auto qtree = qtrees[0];
  EXPECT_TRUE(dynamic_cast<SequentialScanNode*>(qtree.get()) != nullptr);
  auto seqscan = qtree.asInstanceOf<SequentialScanNode>();
  EXPECT_EQ(seqscan->tableName(), "testtable");

  auto constraints = seqscan->constraints();
  EXPECT_EQ(constraints.size(), 3u);

  {
    auto constraint = constraints[0];
    EXPECT_EQ(constraint.column_name, "time");
    EXPECT_TRUE(constraint.type == ScanConstraintType::LESS_THAN);
    EXPECT_EQ(constraint.value.toString(), "1234");
  }

  {
    auto constraint = constraints[1];
    EXPECT_EQ(constraint.column_name, "session_id");
    EXPECT_TRUE(constraint.type == ScanConstraintType::NOT_EQUAL_TO);
    EXPECT_EQ(constraint.value.toString(), "blah");
  }

  {
    auto constraint = constraints[2];
    EXPECT_EQ(constraint.column_name, "time");
    EXPECT_TRUE(constraint.type == ScanConstraintType::GREATER_THAN_OR_EQUAL_TO);
    EXPECT_EQ(constraint.value.toString(), "6666");
  }
  return true;
}

// UNIT-QTREE-008
bool test_sql_qtree_SimpleConstantFolding(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  txn->setTableProvider(
      new CSTableScanProvider(
          "testtable",
          FileUtil::joinPaths(ctx->srcdir, "test/sql_testdata/testtbl.cst")));

  String query = "select 1 + 2 + 3 from testtable where session_id > ucase('fu') + lcase('Bar');";

  csql::Parser parser;
  parser.parse(query.data(), query.size());

  auto qtree_builder = runtime->queryPlanBuilder();
  auto qtrees = qtree_builder->build(
      txn.get(),
      parser.getStatements(),
      txn->getTableProvider());

  EXPECT_EQ(qtrees.size(), 1u);
  auto qtree = qtrees[0];
  EXPECT_TRUE(dynamic_cast<SequentialScanNode*>(qtree.get()) != nullptr);
  auto seqscan = qtree.asInstanceOf<SequentialScanNode>();
  EXPECT_EQ(seqscan->tableName(), "testtable");

  auto where_expr = seqscan->whereExpression();
  EXPECT_FALSE(where_expr.isEmpty());
  EXPECT_EQ(where_expr.get()->toSQL(), "gt(`session_id`,\"FUbar\")");
  return true;
}

// UNIT-QTREE-009
bool test_sql_qtree_PruneConstraints(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  txn->setTableProvider(
      new CSTableScanProvider(
          "testtable",
          FileUtil::joinPaths(ctx->srcdir, "test/sql_testdata/testtbl.cst")));

  String query = "select 1 from testtable where 1000 + 200 + 30 + 4 > time AND session_id != to_string(400 + 44) AND time >= 1111 * 6;";
  csql::Parser parser;
  parser.parse(query.data(), query.size());

  auto qtree_builder = runtime->queryPlanBuilder();
  auto qtrees = qtree_builder->build(
      txn.get(),
      parser.getStatements(),
      txn->getTableProvider());

  EXPECT_EQ(qtrees.size(), 1u);
  auto qtree = qtrees[0];
  EXPECT_TRUE(dynamic_cast<SequentialScanNode*>(qtree.get()) != nullptr);
  auto seqscan = qtree.asInstanceOf<SequentialScanNode>();
  auto where_expr = seqscan->whereExpression().get();

  {
    ScanConstraint constraint;
    constraint.column_name = "time";
    constraint.type = ScanConstraintType::GREATER_THAN_OR_EQUAL_TO;
    constraint.value = SValue::newUInt64(6666);
    RefPtr<ValueExpressionNode> pruned_expr;
    auto rc = QueryTreeUtil::removeConstraintFromPredicate(
        txn.get(),
        where_expr,
        constraint,
        &pruned_expr);

    EXPECT(rc.isSuccess());
    EXPECT_EQ(
        pruned_expr->toSQL(),
        "logical_and(gt(1234,`time`),neq(`session_id`,\"444\"))");
  }

  {
    ScanConstraint constraint;
    constraint.column_name = "time";
    constraint.type = ScanConstraintType::LESS_THAN;
    constraint.value = SValue(SValue::newUInt64(1234));
    RefPtr<ValueExpressionNode> pruned_expr;
    auto rc = QueryTreeUtil::removeConstraintFromPredicate(
        txn.get(),
        where_expr,
        constraint,
        &pruned_expr);

    EXPECT_EQ(
        pruned_expr->toSQL(),
        "logical_and(neq(`session_id`,\"444\"),gte(`time`,6666))");
  }
  return true;
}

// UNIT-QTREE-010
bool test_sql_qtree_Serialization(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  txn->setTableProvider(
      new CSTableScanProvider(
          "testtable",
          FileUtil::joinPaths(ctx->srcdir, "test/sql_testdata/testtbl.cst")));

  String query = "select 1 + 2 + 3 from testtable where session_id > ucase('fu') + lcase('Bar') limit 10;";

  csql::Parser parser;
  parser.parse(query.data(), query.size());

  auto qtree_builder = runtime->queryPlanBuilder();
  auto qtrees = qtree_builder->build(
      txn.get(),
      parser.getStatements(),
      txn->getTableProvider());

  EXPECT_EQ(qtrees.size(), 1u);
  auto qtree = qtrees[0];

  QueryTreeCoder coder(txn.get());

  Buffer buf;
  auto buf_os = BufferOutputStream::fromBuffer(&buf);
  coder.encode(qtree, buf_os.get());

  auto buf_is = BufferInputStream::fromBuffer(&buf);
  auto qtree2 = coder.decode(buf_is.get());

  EXPECT_EQ(qtree->toString(), qtree2->toString());
  return true;
}

// UNIT-QTREE-011
bool test_sql_qtree_SerializationJoinAndSubquery(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  auto tables = mkRef(new TableRepository());
  txn->setTableProvider(tables.get());

  tables->addProvider(
      new backends::csv::CSVTableProvider(
          "customers",
          FileUtil::joinPaths(ctx->srcdir, "test/sql_testdata/testtbl2.csv"),
          '\t'));

  tables->addProvider(
      new backends::csv::CSVTableProvider(
          "orders",
          FileUtil::joinPaths(ctx->srcdir, "test/sql_testdata/testtbl3.csv"),
          '\t'));

  String query =
      "SELECT customers.customername, orders.orderid \
      FROM customers \
      LEFT JOIN orders \
      ON customers.customerid=orders.customerid \
      WHERE true \
      ORDER BY customers.customername;";


  csql::Parser parser;
  parser.parse(query.data(), query.size());

  auto qtree_builder = runtime->queryPlanBuilder();
  auto qtrees = qtree_builder->build(
      txn.get(),
      parser.getStatements(),
      txn->getTableProvider());

  EXPECT_EQ(qtrees.size(), 1u);
  auto qtree = qtrees[0];

  QueryTreeCoder coder(txn.get());

  Buffer buf;
  auto buf_os = BufferOutputStream::fromBuffer(&buf);
  coder.encode(qtree, buf_os.get());

  auto buf_is = BufferInputStream::fromBuffer(&buf);
  auto qtree2 = coder.decode(buf_is.get());

  EXPECT_EQ(qtree->toString(), qtree2->toString());
  return true;
}

// UNIT-QTREE-012
bool test_sql_qtree_CreateTable(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  String query =
    "  CREATE TABLE fnord ("
    "      time DATETIME NOT NULL,"
    "      location STring,"
    "      person REPEATED String,"
    "      temperatur RECORD ("
    "          val1 uint64 NOT NULL,"
    "          val2 REPEATED string"
    "      ),"
    "      some_other REPEATED RECORD ("
    "          val1 uint64 NOT NULL,"
    "          val2 REPEATED STRING"
    "      ),"
    "      PRIMARY KEY (time, myvalue)"
    "  )";

  csql::Parser parser;
  parser.parse(query.data(), query.size());

  auto qtree_builder = runtime->queryPlanBuilder();
  Vector<RefPtr<QueryTreeNode>> qtrees = qtree_builder->build(
      txn.get(),
      parser.getStatements(),
      txn->getTableProvider());

  EXPECT_EQ(qtrees.size(), 1u);
  RefPtr<CreateTableNode> qtree = qtrees[0].asInstanceOf<CreateTableNode>();
  EXPECT_EQ(qtree->getTableName(), "fnord");

  auto table_schema = qtree->getTableSchema();

  auto fcolumns = table_schema.getFlatColumnList();
  EXPECT_EQ(fcolumns.size(), 9u);

  EXPECT_EQ(fcolumns[0]->column_name, "time");
  EXPECT_EQ(fcolumns[0]->column_name, fcolumns[0]->full_column_name);
  EXPECT(fcolumns[0]->column_class == TableSchema::ColumnClass::SCALAR);
  EXPECT_EQ(fcolumns[0]->column_type, "DATETIME");
  EXPECT(fcolumns[0]->column_options == Vector<TableSchema::ColumnOptions> {
    TableSchema::ColumnOptions::NOT_NULL
  });

  EXPECT_EQ(fcolumns[1]->column_name, "location");
  EXPECT_EQ(fcolumns[1]->column_name, fcolumns[1]->full_column_name);
  EXPECT(fcolumns[1]->column_class == TableSchema::ColumnClass::SCALAR);
  EXPECT_EQ(fcolumns[1]->column_type, "STring");
  EXPECT(fcolumns[1]->column_options == Vector<TableSchema::ColumnOptions> {});

  EXPECT_EQ(fcolumns[2]->column_name, "person");
  EXPECT_EQ(fcolumns[2]->column_name, fcolumns[2]->full_column_name);
  EXPECT(fcolumns[2]->column_class == TableSchema::ColumnClass::SCALAR);
  EXPECT_EQ(fcolumns[2]->column_type, "String");
  EXPECT(fcolumns[2]->column_options == Vector<TableSchema::ColumnOptions> {
    TableSchema::ColumnOptions::REPEATED
  });

  EXPECT_EQ(fcolumns[3]->column_name, "temperatur");
  EXPECT_EQ(fcolumns[3]->column_name, fcolumns[3]->full_column_name);
  EXPECT(fcolumns[3]->column_class == TableSchema::ColumnClass::RECORD);
  EXPECT_EQ(fcolumns[3]->column_type, "RECORD");
  EXPECT(fcolumns[3]->column_options == Vector<TableSchema::ColumnOptions> {});
  EXPECT_EQ(fcolumns[3]->column_schema.size(), 2u);

  EXPECT(fcolumns[4]->column_name == "val1");
  EXPECT_EQ(fcolumns[4]->full_column_name, "temperatur.val1");
  EXPECT(fcolumns[4]->column_class == TableSchema::ColumnClass::SCALAR);
  EXPECT_EQ(fcolumns[4]->column_type, "uint64");
  EXPECT(fcolumns[4]->column_options == Vector<TableSchema::ColumnOptions> {
    TableSchema::ColumnOptions::NOT_NULL
  });

  EXPECT(fcolumns[5]->column_name == "val2");
  EXPECT_EQ(fcolumns[5]->full_column_name, "temperatur.val2");
  EXPECT(fcolumns[5]->column_class == TableSchema::ColumnClass::SCALAR);
  EXPECT_EQ(fcolumns[5]->column_type, "string");
  EXPECT(fcolumns[5]->column_options == Vector<TableSchema::ColumnOptions> {
    TableSchema::ColumnOptions::REPEATED
  });

  EXPECT_EQ(fcolumns[6]->column_name, "some_other");
  EXPECT(fcolumns[6]->column_class == TableSchema::ColumnClass::RECORD);
  EXPECT_EQ(fcolumns[6]->column_type, "RECORD");
  EXPECT(fcolumns[6]->column_options == Vector<TableSchema::ColumnOptions> {
    TableSchema::ColumnOptions::REPEATED
  });
  EXPECT_EQ(fcolumns[6]->column_schema.size(), 2u);

  EXPECT(fcolumns[7]->column_name == "val1");
  EXPECT_EQ(fcolumns[7]->full_column_name, "some_other.val1");
  EXPECT(fcolumns[7]->column_class == TableSchema::ColumnClass::SCALAR);
  EXPECT_EQ(fcolumns[7]->column_type, "uint64");
  EXPECT(fcolumns[7]->column_options == Vector<TableSchema::ColumnOptions> {
    TableSchema::ColumnOptions::NOT_NULL
  });

  EXPECT(fcolumns[8]->column_name == "val2");
  EXPECT_EQ(fcolumns[8]->full_column_name, "some_other.val2");
  EXPECT(fcolumns[8]->column_class == TableSchema::ColumnClass::SCALAR);
  EXPECT_EQ(fcolumns[8]->column_type, "STRING");
  EXPECT(fcolumns[8]->column_options == Vector<TableSchema::ColumnOptions> {
    TableSchema::ColumnOptions::REPEATED
  });


  auto columns = table_schema.getColumns();
  EXPECT_EQ(columns.size(), 5u);

  EXPECT_EQ(columns[0]->column_name, "time");
  EXPECT(columns[0]->column_class == TableSchema::ColumnClass::SCALAR);
  EXPECT_EQ(columns[0]->column_type, "DATETIME");
  EXPECT(columns[0]->column_options == Vector<TableSchema::ColumnOptions> {
    TableSchema::ColumnOptions::NOT_NULL
  });

  EXPECT_EQ(columns[1]->column_name, "location");
  EXPECT(columns[1]->column_class == TableSchema::ColumnClass::SCALAR);
  EXPECT_EQ(columns[1]->column_type, "STring");
  EXPECT(columns[1]->column_options == Vector<TableSchema::ColumnOptions> {});

  EXPECT_EQ(columns[2]->column_name, "person");
  EXPECT(columns[2]->column_class == TableSchema::ColumnClass::SCALAR);
  EXPECT_EQ(columns[2]->column_type, "String");
  EXPECT(columns[2]->column_options == Vector<TableSchema::ColumnOptions> {
    TableSchema::ColumnOptions::REPEATED
  });

  EXPECT_EQ(columns[3]->column_name, "temperatur");
  EXPECT(columns[3]->column_class == TableSchema::ColumnClass::RECORD);
  EXPECT_EQ(columns[3]->column_type, "RECORD");
  EXPECT(columns[3]->column_options == Vector<TableSchema::ColumnOptions> {});
  EXPECT_EQ(columns[3]->column_schema.size(), 2u);

  {
    auto scolumns = columns[3]->column_schema;

    EXPECT(scolumns[0]->column_name == "val1");
    EXPECT(scolumns[0]->column_class == TableSchema::ColumnClass::SCALAR);
    EXPECT_EQ(scolumns[0]->column_type, "uint64");
    EXPECT(scolumns[0]->column_options == Vector<TableSchema::ColumnOptions> {
      TableSchema::ColumnOptions::NOT_NULL
    });

    EXPECT(scolumns[1]->column_name == "val2");
    EXPECT(scolumns[1]->column_class == TableSchema::ColumnClass::SCALAR);
    EXPECT_EQ(scolumns[1]->column_type, "string");
    EXPECT(scolumns[1]->column_options == Vector<TableSchema::ColumnOptions> {
      TableSchema::ColumnOptions::REPEATED
    });
  }

  EXPECT_EQ(columns[4]->column_name, "some_other");
  EXPECT(columns[4]->column_class == TableSchema::ColumnClass::RECORD);
  EXPECT_EQ(columns[4]->column_type, "RECORD");
  EXPECT(columns[4]->column_options == Vector<TableSchema::ColumnOptions> {
    TableSchema::ColumnOptions::REPEATED
  });
  EXPECT_EQ(columns[4]->column_schema.size(), 2u);

  {
    auto scolumns = columns[4]->column_schema;

    EXPECT_EQ(scolumns[0]->column_name, "val1");
    EXPECT(scolumns[0]->column_class == TableSchema::ColumnClass::SCALAR);
    EXPECT_EQ(scolumns[0]->column_type, "uint64");
    EXPECT(scolumns[0]->column_options == Vector<TableSchema::ColumnOptions> {
      TableSchema::ColumnOptions::NOT_NULL
    });

    EXPECT_EQ(scolumns[1]->column_name, "val2");
    EXPECT(scolumns[1]->column_class == TableSchema::ColumnClass::SCALAR);
    EXPECT_EQ(scolumns[1]->column_type, "STRING");
    EXPECT(scolumns[1]->column_options == Vector<TableSchema::ColumnOptions> {
      TableSchema::ColumnOptions::REPEATED
    });
  }

  Vector<String> pkey;
  pkey.emplace_back("time");
  pkey.emplace_back("myvalue");
  EXPECT(qtree->getPrimaryKey() == pkey);
  return true;
}

// UNIT-QTREE-013
bool test_sql_qtree_CreateTableWith(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  String query =
    "  CREATE TABLE fnord ("
    "      time DATETIME NOT NULL,"
    "      location string,"
    "      PRIMARY KEY (time, location))"
    "      WITH akey = \"value\" "
    "      AND test.some_key = \"some value\" ";

  csql::Parser parser;
  parser.parse(query.data(), query.size());

  auto qtree_builder = runtime->queryPlanBuilder();
  Vector<RefPtr<QueryTreeNode>> qtrees = qtree_builder->build(
      txn.get(),
      parser.getStatements(),
      txn->getTableProvider());

  EXPECT_EQ(qtrees.size(), 1u);
  RefPtr<CreateTableNode> qtree = qtrees[0].asInstanceOf<CreateTableNode>();
  EXPECT_EQ(qtree->getTableName(), "fnord");

  auto table_schema = qtree->getTableSchema();

  auto fcolumns = table_schema.getFlatColumnList();
  EXPECT_EQ(fcolumns.size(), 2u);

  auto property_list = qtree->getProperties();
  EXPECT_EQ(property_list.size(), 2u);
  EXPECT_EQ(property_list[0].first, "akey");
  EXPECT_EQ(property_list[0].second, "value");
  EXPECT_EQ(property_list[1].first, "test.some_key");
  EXPECT_EQ(property_list[1].second, "some value");
  return true;
}

// UNIT-QTREE-014
bool test_sql_qtree_InsertInto(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  String query = R"(
      INSERT INTO evtbl (
          evtime,
          evid,
          rating,
          is_admin,
          type
      ) VALUES (
          123,
          'xxx',
          1 + 2,
          true,
          null
      );
    )";

  csql::Parser parser;
  parser.parse(query.data(), query.size());

  auto qtree_builder = runtime->queryPlanBuilder();
  Vector<RefPtr<QueryTreeNode>> qtrees = qtree_builder->build(
      txn.get(),
      parser.getStatements(),
      txn->getTableProvider());

  RefPtr<InsertIntoNode> qtree = qtrees[0].asInstanceOf<InsertIntoNode>();
  EXPECT_EQ(qtree->getTableName(), "evtbl");

  auto specs = qtree->getValueSpecs();
  EXPECT_EQ(specs.size(), 5u);

  EXPECT_EQ(specs[0].column, "evtime");
  EXPECT_EQ(specs[1].column, "evid");
  EXPECT_EQ(specs[2].column, "rating");
  EXPECT_EQ(specs[3].column, "is_admin");
  EXPECT_EQ(specs[4].column, "type");

  EXPECT_EQ(specs[0].expr->toSQL(), "123");
  EXPECT_EQ(specs[1].expr->toSQL(), "\"xxx\"");
  EXPECT_EQ(specs[2].expr->toSQL(), "3");
  EXPECT_EQ(specs[3].expr->toSQL(), "true");
  EXPECT_EQ(specs[4].expr->toSQL(), "NULL");
  return true;
}

// UNIT-QTREE-015
//bool test_sql_qtree_InsertShortInto(TestContext* ctx) {
//  auto runtime = Runtime::getDefaultRuntime();
//  auto txn = runtime->newTransaction();
//
//  String query = R"(
//      INSERT evtbl VALUES (
//          123,
//          'xxx',
//          1 + 2,
//          true,
//          null
//      );
//    )";
//
//  csql::Parser parser;
//  parser.parse(query.data(), query.size());
//
//  auto qtree_builder = runtime->queryPlanBuilder();
//  Vector<RefPtr<QueryTreeNode>> qtrees = qtree_builder->build(
//      txn.get(),
//      parser.getStatements(),
//      txn->getTableProvider());
//
//  RefPtr<InsertIntoNode> qtree = qtrees[0].asInstanceOf<InsertIntoNode>();
//  EXPECT_EQ(qtree->getTableName(), "evtbl");
//
//  auto specs = qtree->getValueSpecs();
//  EXPECT_EQ(specs.size(), 5);
//
//  EXPECT_EQ(specs[0].column, "evtime");
//  EXPECT_EQ(specs[1].column, "evid");
//  EXPECT_EQ(specs[2].column, "rating");
//  EXPECT_EQ(specs[3].column, "is_admin");
//  EXPECT_EQ(specs[4].column, "type");
//
//  EXPECT_EQ(specs[0].expr->toSQL(), "123");
//  EXPECT_EQ(specs[1].expr->toSQL(), "\"xxx\"");
//  EXPECT_EQ(specs[2].expr->toSQL(), "3");
//  EXPECT_EQ(specs[3].expr->toSQL(), "true");
//  EXPECT_EQ(specs[4].expr->toSQL(), "NULL");
//});

// UNIT-QTREE-016
bool test_sql_qtree_InsertIntoFromJSON(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  String query = R"(
      INSERT INTO evtbl
      FROM JSON
      '{
          \"evtime\":1464463791,\"evid\":\"xxx\",
          \"products\":[
              {\"id\":1,\"price\":1.23},
              {\"id\":2,\"price\":3.52}
          ]
      }';
  )";

  csql::Parser parser;
  parser.parse(query.data(), query.size());

  auto qtree_builder = runtime->queryPlanBuilder();
  Vector<RefPtr<QueryTreeNode>> qtrees = qtree_builder->build(
      txn.get(),
      parser.getStatements(),
      txn->getTableProvider());

  RefPtr<InsertJSONNode> qtree = qtrees[0].asInstanceOf<InsertJSONNode>();
  EXPECT_EQ(qtree->getTableName(), "evtbl");
  return true;
}

// UNIT-QTREE-017
bool test_sql_qtree_DropTable(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  String query = "DROP TABLE test;";

  csql::Parser parser;
  parser.parse(query.data(), query.size());

  auto qtree_builder = runtime->queryPlanBuilder();
  Vector<RefPtr<QueryTreeNode>> qtrees = qtree_builder->build(
      txn.get(),
      parser.getStatements(),
      txn->getTableProvider());

  RefPtr<DropTableNode> qtree = qtrees[0].asInstanceOf<DropTableNode>();
  EXPECT_EQ(qtree->getTableName(), "test");
  return true;
}

// UNIT-QTREE-018
bool test_sql_qtree_CreateDatabase(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  String query = "CREATE DATABASE test;";

  csql::Parser parser;
  parser.parse(query.data(), query.size());

  auto qtree_builder = runtime->queryPlanBuilder();
  Vector<RefPtr<QueryTreeNode>> qtrees = qtree_builder->build(
      txn.get(),
      parser.getStatements(),
      txn->getTableProvider());

  RefPtr<CreateDatabaseNode> qtree = qtrees[0].asInstanceOf<CreateDatabaseNode>();
  EXPECT_EQ(qtree->getDatabaseName(), "test");
  return true;
}

// UNIT-QTREE-019
bool test_sql_qtree_UseDatabase(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  String query = "USE test;";

  csql::Parser parser;
  parser.parse(query.data(), query.size());

  auto qtree_builder = runtime->queryPlanBuilder();
  Vector<RefPtr<QueryTreeNode>> qtrees = qtree_builder->build(
      txn.get(),
      parser.getStatements(),
      txn->getTableProvider());

  RefPtr<UseDatabaseNode> qtree = qtrees[0].asInstanceOf<UseDatabaseNode>();
  EXPECT_EQ(qtree->getDatabaseName(), "test");
  return true;
}

// UNIT-QTREE-020
bool test_sql_qtree_AlterTable(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  String query = R"(
      ALTER TABLE evtbl
        ADD description REPEATED String,
        DROP place,
        ADD COLUMN product RECORD,
        DROP column version;
  )";

  csql::Parser parser;
  parser.parse(query.data(), query.size());

  auto qtree_builder = runtime->queryPlanBuilder();
  Vector<RefPtr<QueryTreeNode>> qtrees = qtree_builder->build(
      txn.get(),
      parser.getStatements(),
      txn->getTableProvider());

  RefPtr<AlterTableNode> qtree = qtrees[0].asInstanceOf<AlterTableNode>();
  EXPECT_EQ(qtree->getTableName(), "evtbl");

  auto operations = qtree->getOperations();
  EXPECT_EQ(operations.size(), 4u);

  EXPECT(
    operations[0].optype == AlterTableNode::AlterTableOperationType::OP_ADD_COLUMN);
  EXPECT_EQ(operations[0].column_name, "description");
  EXPECT_EQ(operations[0].column_type, "String");
  EXPECT_TRUE(operations[0].is_repeated);
  EXPECT_TRUE(operations[0].is_optional);

  EXPECT(
    operations[1].optype == AlterTableNode::AlterTableOperationType::OP_REMOVE_COLUMN);
  EXPECT_EQ(operations[1].column_name, "place");

  EXPECT(
    operations[2].optype == AlterTableNode::AlterTableOperationType::OP_ADD_COLUMN);
  EXPECT_EQ(operations[2].column_name, "product");
  EXPECT_EQ(operations[2].column_type, "RECORD");
  EXPECT_FALSE(operations[2].is_repeated);
  EXPECT_TRUE(operations[2].is_optional);

  EXPECT(
    operations[3].optype == AlterTableNode::AlterTableOperationType::OP_REMOVE_COLUMN);
  EXPECT_EQ(operations[3].column_name, "version");
  return true;
}

// UNIT-QTREE-021
bool test_sql_qtree_AlterTableSetProperty(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  String query = R"(
      ALTER TABLE evtbl
        ADD description REPEATED String,
        SET PROPERTY disable_split = false,
        SET PROPERTY split_point = 1000;
  )";

  csql::Parser parser;
  parser.parse(query.data(), query.size());

  auto qtree_builder = runtime->queryPlanBuilder();
  Vector<RefPtr<QueryTreeNode>> qtrees = qtree_builder->build(
      txn.get(),
      parser.getStatements(),
      txn->getTableProvider());

  RefPtr<AlterTableNode> qtree = qtrees[0].asInstanceOf<AlterTableNode>();
  EXPECT_EQ(qtree->getTableName(), "evtbl");

  auto properties = qtree->getProperties();
  EXPECT_EQ(properties.size(), 2u);
  EXPECT_EQ(properties[0].first, "disable_split");
  EXPECT_EQ(properties[0].second, "false");

  EXPECT_EQ(properties[1].first, "split_point");
  EXPECT_EQ(properties[1].second, "1000");
  return true;
}

// UNIT-QTREE-022
bool test_sql_qtree_DescribePartitions(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  String query = "DESCRIBE PARTITIONS evtbl";
  csql::Parser parser;
  parser.parse(query.data(), query.size());

  auto qtree_builder = runtime->queryPlanBuilder();
  Vector<RefPtr<QueryTreeNode>> qtrees = qtree_builder->build(
      txn.get(),
      parser.getStatements(),
      txn->getTableProvider());

  RefPtr<DescribePartitionsNode> qtree =
      qtrees[0].asInstanceOf<DescribePartitionsNode>();
  EXPECT_EQ(qtree->tableName(), "evtbl");

  return true;
}

// UNIT-QTREE-023
bool test_sql_qtree_ClusterShowServers(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  String query = "CLUSTER SHOW SERVERS";
  csql::Parser parser;
  parser.parse(query.data(), query.size());

  auto qtree_builder = runtime->queryPlanBuilder();
  Vector<RefPtr<QueryTreeNode>> qtrees = qtree_builder->build(
      txn.get(),
      parser.getStatements(),
      txn->getTableProvider());
  RefPtr<ClusterShowServersNode> qtree =
      qtrees[0].asInstanceOf<ClusterShowServersNode>();
  return true;
}

// UNIT-QTREE-024
bool test_sql_qtree_CreateTableWithPrimaryAndPartitionKey(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  String query =
    "  CREATE TABLE fnord ("
    "      time DATETIME NOT NULL,"
    "      location string,"
    "      PRIMARY KEY (time, location),"
    "      PARTITION KEY (time))";

  csql::Parser parser;
  parser.parse(query.data(), query.size());

  auto qtree_builder = runtime->queryPlanBuilder();
  Vector<RefPtr<QueryTreeNode>> qtrees = qtree_builder->build(
      txn.get(),
      parser.getStatements(),
      txn->getTableProvider());

  EXPECT_EQ(qtrees.size(), 1u);
  RefPtr<CreateTableNode> qtree = qtrees[0].asInstanceOf<CreateTableNode>();
  EXPECT_EQ(qtree->getTableName(), "fnord");

  auto table_schema = qtree->getTableSchema();
  auto fcolumns = table_schema.getFlatColumnList();
  EXPECT_EQ(fcolumns.size(), 2u);

  EXPECT_EQ(qtree->getPrimaryKey().size(), 2u);
  EXPECT_EQ(qtree->getPrimaryKey()[0], "time");
  EXPECT_EQ(qtree->getPrimaryKey()[1], "location");
  EXPECT_EQ(qtree->getPartitionKey(), "time");

  return true;
}

// UNIT-QTREE-025
bool test_sql_qtree_CreateTableWithPrimaryWithoutPartitionKey(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  String query =
    "  CREATE TABLE fnord ("
    "      time DATETIME NOT NULL,"
    "      location string,"
    "      PRIMARY KEY (time, location))";

  csql::Parser parser;
  parser.parse(query.data(), query.size());

  auto qtree_builder = runtime->queryPlanBuilder();
  Vector<RefPtr<QueryTreeNode>> qtrees = qtree_builder->build(
      txn.get(),
      parser.getStatements(),
      txn->getTableProvider());

  EXPECT_EQ(qtrees.size(), 1u);
  RefPtr<CreateTableNode> qtree = qtrees[0].asInstanceOf<CreateTableNode>();
  EXPECT_EQ(qtree->getTableName(), "fnord");

  auto table_schema = qtree->getTableSchema();
  auto fcolumns = table_schema.getFlatColumnList();
  EXPECT_EQ(fcolumns.size(), 2u);

  EXPECT_EQ(qtree->getPrimaryKey().size(), 2u);
  EXPECT_EQ(qtree->getPrimaryKey()[0], "time");
  EXPECT_EQ(qtree->getPrimaryKey()[1], "location");
  EXPECT_TRUE(qtree->getPartitionKey().empty());

  return true;
}

void setup_unit_sql_qtree_tests(TestRepository* repo) {
  SETUP_UNIT_TESTCASE(repo, "UNIT-QTREE-001", sql_qtree, ExtractEqualsConstraint);
  SETUP_UNIT_TESTCASE(repo, "UNIT-QTREE-002", sql_qtree, ExtractNotEqualsConstraint);
  SETUP_UNIT_TESTCASE(repo, "UNIT-QTREE-003", sql_qtree, ExtractLessThanConstraint);
  SETUP_UNIT_TESTCASE(repo, "UNIT-QTREE-004", sql_qtree, ExtractLessThanOrEqualToConstraint);
  SETUP_UNIT_TESTCASE(repo, "UNIT-QTREE-005", sql_qtree, ExtractGreaterThanConstraint);
  SETUP_UNIT_TESTCASE(repo, "UNIT-QTREE-006", sql_qtree, ExtractGreaterThanOrEqualToConstraint);
  SETUP_UNIT_TESTCASE(repo, "UNIT-QTREE-007", sql_qtree, ExtractMultipleConstraints);
  SETUP_UNIT_TESTCASE(repo, "UNIT-QTREE-008", sql_qtree, SimpleConstantFolding);
  SETUP_UNIT_TESTCASE(repo, "UNIT-QTREE-009", sql_qtree, PruneConstraints);
  SETUP_UNIT_TESTCASE(repo, "UNIT-QTREE-010", sql_qtree, Serialization);
  SETUP_UNIT_TESTCASE(repo, "UNIT-QTREE-011", sql_qtree, SerializationJoinAndSubquery);
  SETUP_UNIT_TESTCASE(repo, "UNIT-QTREE-012", sql_qtree, CreateTable);
  SETUP_UNIT_TESTCASE(repo, "UNIT-QTREE-013", sql_qtree, CreateTableWith);
  SETUP_UNIT_TESTCASE(repo, "UNIT-QTREE-014", sql_qtree, InsertInto);
  SETUP_UNIT_TESTCASE(repo, "UNIT-QTREE-016", sql_qtree, InsertIntoFromJSON);
  SETUP_UNIT_TESTCASE(repo, "UNIT-QTREE-017", sql_qtree, DropTable);
  SETUP_UNIT_TESTCASE(repo, "UNIT-QTREE-018", sql_qtree, CreateDatabase);
  SETUP_UNIT_TESTCASE(repo, "UNIT-QTREE-019", sql_qtree, UseDatabase);
  SETUP_UNIT_TESTCASE(repo, "UNIT-QTREE-020", sql_qtree, AlterTable);
  SETUP_UNIT_TESTCASE(repo, "UNIT-QTREE-021", sql_qtree, AlterTableSetProperty);
  SETUP_UNIT_TESTCASE(repo, "UNIT-QTREE-022", sql_qtree, DescribePartitions);
  SETUP_UNIT_TESTCASE(repo, "UNIT-QTREE-023", sql_qtree, ClusterShowServers);
  SETUP_UNIT_TESTCASE(repo, "UNIT-QTREE-024", sql_qtree, CreateTableWithPrimaryAndPartitionKey);
  SETUP_UNIT_TESTCASE(repo, "UNIT-QTREE-025", sql_qtree, CreateTableWithPrimaryWithoutPartitionKey);
}

} // namespace unit
} // namespace test
} // namespace eventql

