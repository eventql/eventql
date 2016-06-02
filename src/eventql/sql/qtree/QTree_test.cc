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
#include <eventql/util/stdtypes.h>
#include <eventql/util/exception.h>
#include <eventql/util/wallclock.h>
#include <eventql/util/test/unittest.h>
#include "eventql/sql/svalue.h"
#include "eventql/sql/runtime/defaultruntime.h"
#include "eventql/sql/qtree/SequentialScanNode.h"
#include "eventql/sql/qtree/ColumnReferenceNode.h"
#include "eventql/sql/qtree/CallExpressionNode.h"
#include "eventql/sql/qtree/LiteralExpressionNode.h"
#include "eventql/sql/qtree/QueryTreeUtil.h"
#include "eventql/sql/qtree/qtree_coder.h"
#include "eventql/sql/qtree/nodes/alter_table.h"
#include "eventql/sql/qtree/nodes/create_table.h"
#include "eventql/sql/qtree/nodes/insert_into.h"
#include "eventql/sql/qtree/nodes/insert_json.h"
#include "eventql/sql/CSTableScanProvider.h"
#include "eventql/sql/backends/csv/CSVTableProvider.h"

#include "eventql/eventql.h"
using namespace csql;

UNIT_TEST(QTreeTest);

TEST_CASE(QTreeTest, TestExtractEqualsConstraint, [] () {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  txn->setTableProvider(
      new CSTableScanProvider(
          "testtable",
          "eventql/sql/testdata/testtbl.cst"));

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

    EXPECT_EQ(qtrees.size(), 1);
    auto qtree = qtrees[0];
    EXPECT_TRUE(dynamic_cast<SequentialScanNode*>(qtree.get()) != nullptr);
    auto seqscan = qtree.asInstanceOf<SequentialScanNode>();
    EXPECT_EQ(seqscan->tableName(), "testtable");

    auto constraints = seqscan->constraints();
    EXPECT_EQ(constraints.size(), 1);

    auto constraint = constraints[0];
    EXPECT_EQ(constraint.column_name, "time");
    EXPECT_TRUE(constraint.type == ScanConstraintType::EQUAL_TO);
    EXPECT_EQ(constraint.value.getString(), "1234");
  }
});

TEST_CASE(QTreeTest, TestExtractNotEqualsConstraint, [] () {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  txn->setTableProvider(
      new CSTableScanProvider(
          "testtable",
          "eventql/sql/testdata/testtbl.cst"));

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

    EXPECT_EQ(qtrees.size(), 1);
    auto qtree = qtrees[0];
    EXPECT_TRUE(dynamic_cast<SequentialScanNode*>(qtree.get()) != nullptr);
    auto seqscan = qtree.asInstanceOf<SequentialScanNode>();
    EXPECT_EQ(seqscan->tableName(), "testtable");

    auto constraints = seqscan->constraints();
    EXPECT_EQ(constraints.size(), 1);

    auto constraint = constraints[0];
    EXPECT_EQ(constraint.column_name, "time");
    EXPECT_TRUE(constraint.type == ScanConstraintType::NOT_EQUAL_TO);
    EXPECT_EQ(constraint.value.getString(), "1234");
  }
});

TEST_CASE(QTreeTest, TestExtractLessThanConstraint, [] () {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  txn->setTableProvider(
      new CSTableScanProvider(
          "testtable",
          "eventql/sql/testdata/testtbl.cst"));

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

    EXPECT_EQ(qtrees.size(), 1);
    auto qtree = qtrees[0];
    EXPECT_TRUE(dynamic_cast<SequentialScanNode*>(qtree.get()) != nullptr);
    auto seqscan = qtree.asInstanceOf<SequentialScanNode>();
    EXPECT_EQ(seqscan->tableName(), "testtable");

    auto constraints = seqscan->constraints();
    EXPECT_EQ(constraints.size(), 1);

    auto constraint = constraints[0];
    EXPECT_EQ(constraint.column_name, "time");
    EXPECT_TRUE(constraint.type == ScanConstraintType::LESS_THAN);
    EXPECT_EQ(constraint.value.getString(), "1234");
  }
});

TEST_CASE(QTreeTest, TestExtractLessThanOrEqualToConstraint, [] () {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  txn->setTableProvider(
      new CSTableScanProvider(
          "testtable",
          "eventql/sql/testdata/testtbl.cst"));

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

    EXPECT_EQ(qtrees.size(), 1);
    auto qtree = qtrees[0];
    EXPECT_TRUE(dynamic_cast<SequentialScanNode*>(qtree.get()) != nullptr);
    auto seqscan = qtree.asInstanceOf<SequentialScanNode>();
    EXPECT_EQ(seqscan->tableName(), "testtable");

    auto constraints = seqscan->constraints();
    EXPECT_EQ(constraints.size(), 1);

    auto constraint = constraints[0];
    EXPECT_EQ(constraint.column_name, "time");
    EXPECT_TRUE(constraint.type == ScanConstraintType::LESS_THAN_OR_EQUAL_TO);
    EXPECT_EQ(constraint.value.getString(), "1234");
  }
});

TEST_CASE(QTreeTest, TestExtractGreaterThanConstraint, [] () {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  txn->setTableProvider(
      new CSTableScanProvider(
          "testtable",
          "eventql/sql/testdata/testtbl.cst"));

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

    EXPECT_EQ(qtrees.size(), 1);
    auto qtree = qtrees[0];
    EXPECT_TRUE(dynamic_cast<SequentialScanNode*>(qtree.get()) != nullptr);
    auto seqscan = qtree.asInstanceOf<SequentialScanNode>();
    EXPECT_EQ(seqscan->tableName(), "testtable");

    auto constraints = seqscan->constraints();
    EXPECT_EQ(constraints.size(), 1);

    auto constraint = constraints[0];
    EXPECT_EQ(constraint.column_name, "time");
    EXPECT_TRUE(constraint.type == ScanConstraintType::GREATER_THAN);
    EXPECT_EQ(constraint.value.getString(), "1234");
  }
});

TEST_CASE(QTreeTest, TestExtractGreaterThanOrEqualToConstraint, [] () {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  txn->setTableProvider(
      new CSTableScanProvider(
          "testtable",
          "eventql/sql/testdata/testtbl.cst"));

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

    EXPECT_EQ(qtrees.size(), 1);
    auto qtree = qtrees[0];
    EXPECT_TRUE(dynamic_cast<SequentialScanNode*>(qtree.get()) != nullptr);
    auto seqscan = qtree.asInstanceOf<SequentialScanNode>();
    EXPECT_EQ(seqscan->tableName(), "testtable");

    auto constraints = seqscan->constraints();
    EXPECT_EQ(constraints.size(), 1);

    auto constraint = constraints[0];
    EXPECT_EQ(constraint.column_name, "time");
    EXPECT_TRUE(constraint.type == ScanConstraintType::GREATER_THAN_OR_EQUAL_TO);
    EXPECT_EQ(constraint.value.getString(), "1234");
  }
});

TEST_CASE(QTreeTest, TestExtractMultipleConstraints, [] () {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  txn->setTableProvider(
      new CSTableScanProvider(
          "testtable",
          "eventql/sql/testdata/testtbl.cst"));

  String query = "select 1 from testtable where 1000 + 200 + 30 + 4 > time AND session_id != 400 + 44 AND time >= 1111 * 6;";

  csql::Parser parser;
  parser.parse(query.data(), query.size());

  auto qtree_builder = runtime->queryPlanBuilder();
  auto qtrees = qtree_builder->build(
      txn.get(),
      parser.getStatements(),
      txn->getTableProvider());

  EXPECT_EQ(qtrees.size(), 1);
  auto qtree = qtrees[0];
  EXPECT_TRUE(dynamic_cast<SequentialScanNode*>(qtree.get()) != nullptr);
  auto seqscan = qtree.asInstanceOf<SequentialScanNode>();
  EXPECT_EQ(seqscan->tableName(), "testtable");

  auto constraints = seqscan->constraints();
  EXPECT_EQ(constraints.size(), 3);

  {
    auto constraint = constraints[0];
    EXPECT_EQ(constraint.column_name, "time");
    EXPECT_TRUE(constraint.type == ScanConstraintType::LESS_THAN);
    EXPECT_EQ(constraint.value.getString(), "1234");
  }

  {
    auto constraint = constraints[1];
    EXPECT_EQ(constraint.column_name, "session_id");
    EXPECT_TRUE(constraint.type == ScanConstraintType::NOT_EQUAL_TO);
    EXPECT_EQ(constraint.value.getString(), "444");
  }

  {
    auto constraint = constraints[2];
    EXPECT_EQ(constraint.column_name, "time");
    EXPECT_TRUE(constraint.type == ScanConstraintType::GREATER_THAN_OR_EQUAL_TO);
    EXPECT_EQ(constraint.value.getString(), "6666");
  }
});

TEST_CASE(QTreeTest, TestSimpleConstantFolding, [] () {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  txn->setTableProvider(
      new CSTableScanProvider(
          "testtable",
          "eventql/sql/testdata/testtbl.cst"));

  String query = "select 1 + 2 + 3 from testtable where time > ucase('fu') + lcase('Bar');";

  csql::Parser parser;
  parser.parse(query.data(), query.size());

  auto qtree_builder = runtime->queryPlanBuilder();
  auto qtrees = qtree_builder->build(
      txn.get(),
      parser.getStatements(),
      txn->getTableProvider());

  EXPECT_EQ(qtrees.size(), 1);
  auto qtree = qtrees[0];
  EXPECT_TRUE(dynamic_cast<SequentialScanNode*>(qtree.get()) != nullptr);
  auto seqscan = qtree.asInstanceOf<SequentialScanNode>();
  EXPECT_EQ(seqscan->tableName(), "testtable");

  auto where_expr = seqscan->whereExpression();
  EXPECT_FALSE(where_expr.isEmpty());
  EXPECT_EQ(where_expr.get()->toSQL(), "gt(`time`,\"FUbar\")");
});

TEST_CASE(QTreeTest, TestPruneConstraints, [] () {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  txn->setTableProvider(
      new CSTableScanProvider(
          "testtable",
          "eventql/sql/testdata/testtbl.cst"));

  String query = "select 1 from testtable where 1000 + 200 + 30 + 4 > time AND session_id != 400 + 44 AND time >= 1111 * 6;";
  csql::Parser parser;
  parser.parse(query.data(), query.size());

  auto qtree_builder = runtime->queryPlanBuilder();
  auto qtrees = qtree_builder->build(
      txn.get(),
      parser.getStatements(),
      txn->getTableProvider());

  EXPECT_EQ(qtrees.size(), 1);
  auto qtree = qtrees[0];
  EXPECT_TRUE(dynamic_cast<SequentialScanNode*>(qtree.get()) != nullptr);
  auto seqscan = qtree.asInstanceOf<SequentialScanNode>();
  auto where_expr = seqscan->whereExpression().get();

  EXPECT_EQ(
      where_expr->toSQL(),
      "logical_and(logical_and(gt(1234,`time`),neq(`session_id`,444)),gte(`time`,6666))");

  {
    ScanConstraint constraint;
    constraint.column_name = "time";
    constraint.type = ScanConstraintType::GREATER_THAN_OR_EQUAL_TO;
    constraint.value = SValue(SValue::IntegerType(6666));
    auto pruned_expr = QueryTreeUtil::removeConstraintFromPredicate(
        where_expr,
        constraint);

    EXPECT_EQ(
        pruned_expr->toSQL(),
        "logical_and(logical_and(gt(1234,`time`),neq(`session_id`,444)),true)");
  }

  {
    ScanConstraint constraint;
    constraint.column_name = "time";
    constraint.type = ScanConstraintType::LESS_THAN;
    constraint.value = SValue(SValue::IntegerType(1234));
    auto pruned_expr = QueryTreeUtil::removeConstraintFromPredicate(
        where_expr,
        constraint);

    EXPECT_EQ(
        pruned_expr->toSQL(),
        "logical_and(logical_and(true,neq(`session_id`,444)),gte(`time`,6666))");
  }
});

TEST_CASE(QTreeTest, TestSerialization, [] () {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  txn->setTableProvider(
      new CSTableScanProvider(
          "testtable",
          "eventql/sql/testdata/testtbl.cst"));

  String query = "select 1 + 2 + 3 from testtable where time > ucase('fu') + lcase('Bar') limit 10;";

  csql::Parser parser;
  parser.parse(query.data(), query.size());

  auto qtree_builder = runtime->queryPlanBuilder();
  auto qtrees = qtree_builder->build(
      txn.get(),
      parser.getStatements(),
      txn->getTableProvider());

  EXPECT_EQ(qtrees.size(), 1);
  auto qtree = qtrees[0];

  QueryTreeCoder coder(txn.get());

  Buffer buf;
  auto buf_os = BufferOutputStream::fromBuffer(&buf);
  coder.encode(qtree, buf_os.get());

  auto buf_is = BufferInputStream::fromBuffer(&buf);
  auto qtree2 = coder.decode(buf_is.get());

  EXPECT_EQ(qtree->toString(), qtree2->toString());
});

TEST_CASE(QTreeTest, TestSerializationJoinAndSubquery, [] () {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  auto tables = mkRef(new TableRepository());
  txn->setTableProvider(tables.get());

  tables->addProvider(
    new backends::csv::CSVTableProvider(
        "customers",
        "eventql/sql/testdata/testtbl2.csv",
        '\t'));

  tables->addProvider(
      new backends::csv::CSVTableProvider(
          "orders",
          "eventql/sql/testdata/testtbl3.csv",
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

  EXPECT_EQ(qtrees.size(), 1);
  auto qtree = qtrees[0];

  QueryTreeCoder coder(txn.get());

  Buffer buf;
  auto buf_os = BufferOutputStream::fromBuffer(&buf);
  coder.encode(qtree, buf_os.get());

  auto buf_is = BufferInputStream::fromBuffer(&buf);
  auto qtree2 = coder.decode(buf_is.get());

  EXPECT_EQ(qtree->toString(), qtree2->toString());
});

TEST_CASE(QTreeTest, TestCreateTable, [] () {
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

  EXPECT_EQ(qtrees.size(), 1);
  RefPtr<CreateTableNode> qtree = qtrees[0].asInstanceOf<CreateTableNode>();
  EXPECT_EQ(qtree->getTableName(), "fnord");

  auto table_schema = qtree->getTableSchema();

  auto fcolumns = table_schema.getFlatColumnList();
  EXPECT_EQ(fcolumns.size(), 9);

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
  EXPECT_EQ(fcolumns[3]->column_schema.size(), 2);

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
  EXPECT_EQ(fcolumns[6]->column_schema.size(), 2);

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
  EXPECT_EQ(columns.size(), 5);

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
  EXPECT_EQ(columns[3]->column_schema.size(), 2);

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
  EXPECT_EQ(columns[4]->column_schema.size(), 2);

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
});

TEST_CASE(QTreeTest, TestInsertInto, [] () {
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
  EXPECT_EQ(specs.size(), 5);

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
});

TEST_CASE(QTreeTest, TestInsertIntoFromJSON, [] () {
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
});

TEST_CASE(QTreeTest, TestAlterTable, [] () {
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
  EXPECT_EQ(operations.size(), 4);

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
});
