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
#include <iomanip>
#include <sstream>
#include "eventql/eventql.h"
#include <eventql/util/stdtypes.h>
#include <eventql/util/exception.h>
#include <eventql/util/wallclock.h>
#include <eventql/sql/parser/parser.h>
#include <eventql/sql/parser/token.h>
#include <eventql/sql/parser/tokenize.h>
#include "eventql/sql/runtime/defaultruntime.h"
#include "../unit_test.h"

namespace eventql {
namespace test {
namespace unit {

using namespace csql;

static Parser parseTestQuery(const char* query) {
  Parser parser;
  parser.parse(query, strlen(query));
  return parser;
}

// UNIT-SQLPARSER-001
bool test_sql_parser_SimpleValueExpression(TestContext* ctx) {
  auto parser = parseTestQuery("SELECT 23 + 5.123 FROM sometable;");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(*stmt == ASTNode::T_SELECT);
  EXPECT(stmt->getChildren().size() == 2);
  const auto& sl = stmt->getChildren()[0];
  EXPECT(*sl == ASTNode::T_SELECT_LIST);
  EXPECT(sl->getChildren().size() == 1);
  auto derived = sl->getChildren()[0];
  EXPECT(*derived == ASTNode::T_DERIVED_COLUMN);
  EXPECT(derived->getChildren().size() == 1);
  auto expr = derived->getChildren()[0];
  EXPECT(*expr == ASTNode::T_ADD_EXPR);
  EXPECT(expr->getChildren().size() == 2);
  EXPECT(*expr->getChildren()[0] == ASTNode::T_LITERAL);
  EXPECT(*expr->getChildren()[0]->getToken() == Token::T_NUMERIC);
  EXPECT(*expr->getChildren()[0]->getToken() == "23");
  EXPECT(*expr->getChildren()[1] == ASTNode::T_LITERAL);
  EXPECT(*expr->getChildren()[1]->getToken() == Token::T_NUMERIC);
  EXPECT(*expr->getChildren()[1]->getToken() == "5.123");
  const auto& from = stmt->getChildren()[1];
  EXPECT(*from == ASTNode::T_FROM);
  return true;
}

// UNIT-SQLPARSER-002
bool test_sql_parser_ArithmeticValueExpression(TestContext* ctx) {
  auto parser = parseTestQuery("SELECT 1 + 2 / 3;");
  EXPECT(parser.getStatements().size() == 1);
  auto expr = parser.getStatements()[0]
      ->getChildren()[0]->getChildren()[0]->getChildren()[0];
  EXPECT(*expr == ASTNode::T_ADD_EXPR);
  EXPECT(expr->getChildren().size() == 2);
  EXPECT(*expr->getChildren()[0] == ASTNode::T_LITERAL);
  EXPECT(*expr->getChildren()[0]->getToken() == Token::T_NUMERIC);
  EXPECT(*expr->getChildren()[0]->getToken() == "1");
  EXPECT(*expr->getChildren()[1] == ASTNode::T_DIV_EXPR);
  EXPECT(expr->getChildren()[1]->getChildren().size() == 2);
  return true;
}

// UNIT-SQLPARSER-003
bool test_sql_parser_ArithmeticValueExpressionParens(TestContext* ctx) {
  auto parser = parseTestQuery("SELECT (1 * 2) + 3;");
  EXPECT(parser.getStatements().size() == 1);
  auto expr = parser.getStatements()[0]
      ->getChildren()[0]->getChildren()[0]->getChildren()[0];
  EXPECT(*expr == ASTNode::T_ADD_EXPR);
  EXPECT(expr->getChildren().size() == 2);
  EXPECT(*expr->getChildren()[0] == ASTNode::T_MUL_EXPR);
  EXPECT(expr->getChildren()[0]->getChildren().size() == 2);
  EXPECT(*expr->getChildren()[1] == ASTNode::T_LITERAL);
  EXPECT(*expr->getChildren()[1]->getToken() == Token::T_NUMERIC);
  EXPECT(*expr->getChildren()[1]->getToken() == "3");
  return true;
}

// UNIT-SQLPARSER-004
bool test_sql_parser_ParseEqual(TestContext* ctx) {
  auto parser = parseTestQuery("SELECT 2=5;");
  EXPECT(parser.getStatements().size() == 1);
  auto expr = parser.getStatements()[0]
      ->getChildren()[0]->getChildren()[0]->getChildren()[0];
  EXPECT(*expr == ASTNode::T_EQ_EXPR);
  EXPECT(expr->getChildren().size() == 2);
  EXPECT(*expr->getChildren()[0] == ASTNode::T_LITERAL);
  EXPECT(*expr->getChildren()[0]->getToken() == Token::T_NUMERIC);
  EXPECT(*expr->getChildren()[0]->getToken() == "2");
  EXPECT(*expr->getChildren()[1] == ASTNode::T_LITERAL);
  EXPECT(*expr->getChildren()[1]->getToken() == Token::T_NUMERIC);
  EXPECT(*expr->getChildren()[1]->getToken() == "5");
  return true;
}

// UNIT-SQLPARSER-005
bool test_sql_parser_ParseNotEqual(TestContext* ctx) {
  auto parser = parseTestQuery("SELECT 2!=5;");
  EXPECT(parser.getStatements().size() == 1);
  auto expr = parser.getStatements()[0]
      ->getChildren()[0]->getChildren()[0]->getChildren()[0];
  EXPECT(*expr == ASTNode::T_NEQ_EXPR);
  EXPECT(expr->getChildren().size() == 2);
  EXPECT(*expr->getChildren()[0] == ASTNode::T_LITERAL);
  EXPECT(*expr->getChildren()[0]->getToken() == Token::T_NUMERIC);
  EXPECT(*expr->getChildren()[0]->getToken() == "2");
  EXPECT(*expr->getChildren()[1] == ASTNode::T_LITERAL);
  EXPECT(*expr->getChildren()[1]->getToken() == Token::T_NUMERIC);
  EXPECT(*expr->getChildren()[1]->getToken() == "5");
  return true;
}

// UNIT-SQLPARSER-006
bool test_sql_parser_ParseGreaterThan(TestContext* ctx) {
  auto parser = parseTestQuery("SELECT 2>5;");
  EXPECT(parser.getStatements().size() == 1);
  auto expr = parser.getStatements()[0]
      ->getChildren()[0]->getChildren()[0]->getChildren()[0];
  EXPECT(*expr == ASTNode::T_GT_EXPR);
  EXPECT(expr->getChildren().size() == 2);
  EXPECT(*expr->getChildren()[0] == ASTNode::T_LITERAL);
  EXPECT(*expr->getChildren()[0]->getToken() == Token::T_NUMERIC);
  EXPECT(*expr->getChildren()[0]->getToken() == "2");
  EXPECT(*expr->getChildren()[1] == ASTNode::T_LITERAL);
  EXPECT(*expr->getChildren()[1]->getToken() == Token::T_NUMERIC);
  EXPECT(*expr->getChildren()[1]->getToken() == "5");
  return true;
}

// UNIT-SQLPARSER-007
bool test_sql_parser_ParseGreaterThanEqual(TestContext* ctx) {
  auto parser = parseTestQuery("SELECT 2>=5;");
  EXPECT(parser.getStatements().size() == 1);
  auto expr = parser.getStatements()[0]
      ->getChildren()[0]->getChildren()[0]->getChildren()[0];
  EXPECT(*expr == ASTNode::T_GTE_EXPR);
  EXPECT(expr->getChildren().size() == 2);
  EXPECT(*expr->getChildren()[0] == ASTNode::T_LITERAL);
  EXPECT(*expr->getChildren()[0]->getToken() == Token::T_NUMERIC);
  EXPECT(*expr->getChildren()[0]->getToken() == "2");
  EXPECT(*expr->getChildren()[1] == ASTNode::T_LITERAL);
  EXPECT(*expr->getChildren()[1]->getToken() == Token::T_NUMERIC);
  EXPECT(*expr->getChildren()[1]->getToken() == "5");
  return true;
}

// UNIT-SQLPARSER-008
bool test_sql_parser_ArithmeticValueExpressionPrecedence(TestContext* ctx) {
  {
    auto parser = parseTestQuery("SELECT 1 * 2 + 3;");
    EXPECT(parser.getStatements().size() == 1);
    auto expr = parser.getStatements()[0]
        ->getChildren()[0]->getChildren()[0]->getChildren()[0];
    EXPECT(*expr == ASTNode::T_ADD_EXPR);
    EXPECT(expr->getChildren().size() == 2);
    EXPECT(*expr->getChildren()[0] == ASTNode::T_MUL_EXPR);
    EXPECT(expr->getChildren()[0]->getChildren().size() == 2);
    EXPECT(*expr->getChildren()[1] == ASTNode::T_LITERAL);
    EXPECT(*expr->getChildren()[1]->getToken() == Token::T_NUMERIC);
    EXPECT(*expr->getChildren()[1]->getToken() == "3");
  }
  {
    auto parser = parseTestQuery("SELECT 1 + 2 * 3;");
    EXPECT(parser.getStatements().size() == 1);
    auto expr = parser.getStatements()[0]
        ->getChildren()[0]->getChildren()[0]->getChildren()[0];
    EXPECT(*expr == ASTNode::T_ADD_EXPR);
    EXPECT(expr->getChildren().size() == 2);
    EXPECT(*expr->getChildren()[0] == ASTNode::T_LITERAL);
    EXPECT(*expr->getChildren()[0]->getToken() == Token::T_NUMERIC);
    EXPECT(*expr->getChildren()[0]->getToken() == "1");
    EXPECT(*expr->getChildren()[1] == ASTNode::T_MUL_EXPR);
    EXPECT(expr->getChildren()[1]->getChildren().size() == 2);
  }
  return true;
}

// UNIT-SQLPARSER-009
bool test_sql_parser_MethodCallValueExpression(TestContext* ctx) {
  auto parser = parseTestQuery("SELECT 1 + sum(23, 4 + 1) FROM sometable;");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(*stmt == ASTNode::T_SELECT);
  EXPECT(stmt->getChildren().size() == 2);
  const auto& sl = stmt->getChildren()[0];
  EXPECT(*sl == ASTNode::T_SELECT_LIST);
  EXPECT(sl->getChildren().size() == 1);
  auto derived = sl->getChildren()[0];
  EXPECT(*derived == ASTNode::T_DERIVED_COLUMN);
  EXPECT(derived->getChildren().size() == 1);
  auto expr = derived->getChildren()[0];
  EXPECT(*expr == ASTNode::T_ADD_EXPR);
  EXPECT(expr->getChildren().size() == 2);
  EXPECT(*expr->getChildren()[0] == ASTNode::T_LITERAL);
  EXPECT(*expr->getChildren()[0]->getToken() == Token::T_NUMERIC);
  EXPECT(*expr->getChildren()[0]->getToken() == "1");
  auto mcall = expr->getChildren()[1];
  EXPECT(*mcall == ASTNode::T_METHOD_CALL);
  EXPECT(*mcall->getToken() == Token::T_IDENTIFIER);
  EXPECT(*mcall->getToken() == "sum");
  EXPECT(mcall->getChildren().size() == 2);
  EXPECT(*mcall->getChildren()[0] == ASTNode::T_LITERAL);
  EXPECT(*mcall->getChildren()[0]->getToken() == Token::T_NUMERIC);
  EXPECT(*mcall->getChildren()[0]->getToken() == "23");
  EXPECT(*mcall->getChildren()[1] == ASTNode::T_ADD_EXPR);
  EXPECT(mcall->getChildren()[1]->getChildren().size() == 2);
  const auto& from = stmt->getChildren()[1];
  EXPECT(*from == ASTNode::T_FROM);
  return true;
}

// UNIT-SQLPARSER-010
bool test_sql_parser_NegatedValueExpression(TestContext* ctx) {
  auto parser = parseTestQuery("SELECT -(23 + 5.123) AS fucol FROM tbl;");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(*stmt == ASTNode::T_SELECT);
  EXPECT(stmt->getChildren().size() == 2);
  const auto& sl = stmt->getChildren()[0];
  EXPECT(*sl == ASTNode::T_SELECT_LIST);
  EXPECT(sl->getChildren().size() == 1);
  auto derived = sl->getChildren()[0];
  EXPECT(*derived == ASTNode::T_DERIVED_COLUMN);
  EXPECT(derived->getChildren().size() == 2);
  auto neg_expr = derived->getChildren()[0];
  EXPECT(*neg_expr == ASTNode::T_NEGATE_EXPR);
  EXPECT(neg_expr->getChildren().size() == 1);
  auto expr = neg_expr->getChildren()[0];
  EXPECT(*expr == ASTNode::T_ADD_EXPR);
  EXPECT(expr->getChildren().size() == 2);
  EXPECT(*expr->getChildren()[0] == ASTNode::T_LITERAL);
  EXPECT(*expr->getChildren()[0]->getToken() == Token::T_NUMERIC);
  EXPECT(*expr->getChildren()[0]->getToken() == "23");
  EXPECT(*expr->getChildren()[1] == ASTNode::T_LITERAL);
  EXPECT(*expr->getChildren()[1]->getToken() == Token::T_NUMERIC);
  EXPECT(*expr->getChildren()[1]->getToken() == "5.123");
  auto col_name = derived->getChildren()[1];
  EXPECT(*col_name == ASTNode::T_COLUMN_ALIAS);
  EXPECT(*col_name->getToken() == Token::T_IDENTIFIER);
  EXPECT(*col_name->getToken() == "fucol");
  const auto& from = stmt->getChildren()[1];
  EXPECT(*from == ASTNode::T_FROM);
  return true;
}

// UNIT-SQLPARSER-011
bool test_sql_parser_SelectAll(TestContext* ctx) {
  auto parser = parseTestQuery("SELECT * FROM sometable;");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(*stmt == ASTNode::T_SELECT);
  EXPECT(stmt->getChildren().size() == 2);
  const auto& sl = stmt->getChildren()[0];
  EXPECT(*sl == ASTNode::T_SELECT_LIST);
  EXPECT(sl->getChildren().size() == 1);
  EXPECT(*sl->getChildren()[0] == ASTNode::T_ALL);
  EXPECT(sl->getChildren()[0]->getToken() == nullptr);
  const auto& from = stmt->getChildren()[1];
  EXPECT(*from == ASTNode::T_FROM);
  return true;
}

// UNIT-SQLPARSER-012
bool test_sql_parser_SelectTableWildcard(TestContext* ctx) {
  auto parser = parseTestQuery("SELECT mytablex.* FROM sometable;");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(*stmt == ASTNode::T_SELECT);
  EXPECT(stmt->getChildren().size() == 2);
  const auto& sl = stmt->getChildren()[0];
  EXPECT(*sl == ASTNode::T_SELECT_LIST);
  EXPECT(sl->getChildren().size() == 1);
  const auto& all = sl->getChildren()[0];
  EXPECT(*all == ASTNode::T_ALL);
  EXPECT(all->getToken() != nullptr);
  EXPECT(*all->getToken() == Token::T_IDENTIFIER);
  EXPECT(*all->getToken() == "mytablex");
  const auto& from = stmt->getChildren()[1];
  EXPECT(*from == ASTNode::T_FROM);
  return true;
}

// UNIT-SQLPARSER-013
bool test_sql_parser_QuotedTableName(TestContext* ctx) {
  auto parser = parseTestQuery("SELECT * FROM `some.table`;");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(*stmt == ASTNode::T_SELECT);
  EXPECT(stmt->getChildren().size() == 2);
  const auto& from = stmt->getChildren()[1];
  EXPECT(*from == ASTNode::T_FROM);
  EXPECT(from->getChildren().size() == 1);
  const auto& tlbname = from->getChildren()[0];
  EXPECT_EQ(tlbname->getToken()->getString(), "some.table");
  return true;
}

// UNIT-SQLPARSER-014
bool test_sql_parser_NestedTableName(TestContext* ctx) {
  auto parser = parseTestQuery("SELECT * FROM some.tbl;");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(*stmt == ASTNode::T_SELECT);
  EXPECT(stmt->getChildren().size() == 2);
  const auto& from = stmt->getChildren()[1];
  EXPECT(*from == ASTNode::T_FROM);
  EXPECT(from->getChildren().size() == 1);
  const auto& tlbname = from->getChildren()[0];
  EXPECT_EQ(tlbname->getToken()->getString(), "some.tbl");
  return true;
}

// UNIT-SQLPARSER-015
bool test_sql_parser_SelectDerivedColumn(TestContext* ctx) {
  auto parser = parseTestQuery("SELECT somecol AS another FROM sometable;");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(*stmt == ASTNode::T_SELECT);
  EXPECT(stmt->getChildren().size() == 2);
  const auto& sl = stmt->getChildren()[0];
  EXPECT(*sl == ASTNode::T_SELECT_LIST);
  EXPECT(sl->getChildren().size() == 1);
  const auto& derived = sl->getChildren()[0];
  EXPECT(*derived == ASTNode::T_DERIVED_COLUMN);
  EXPECT(derived->getChildren().size() == 2);
  EXPECT(*derived->getChildren()[0] == ASTNode::T_COLUMN_NAME);
  EXPECT(*derived->getChildren()[0]->getToken() == Token::T_IDENTIFIER);
  EXPECT(*derived->getChildren()[0]->getToken() == "somecol");
  EXPECT(*derived->getChildren()[1] == ASTNode::T_COLUMN_ALIAS);
  EXPECT(*derived->getChildren()[1]->getToken() == Token::T_IDENTIFIER);
  EXPECT(*derived->getChildren()[1]->getToken() == "another");
  const auto& from = stmt->getChildren()[1];
  EXPECT(*from == ASTNode::T_FROM);
  return true;
}

// UNIT-SQLPARSER-016
bool test_sql_parser_SelectDerivedColumnWithTableName(TestContext* ctx) {
  auto parser = parseTestQuery("SELECT tbl.col AS another FROM sometable;");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(*stmt == ASTNode::T_SELECT);
  EXPECT(stmt->getChildren().size() == 2);
  const auto& sl = stmt->getChildren()[0];
  EXPECT(*sl == ASTNode::T_SELECT_LIST);
  EXPECT(sl->getChildren().size() == 1);
  const auto& derived = sl->getChildren()[0];
  EXPECT(*derived == ASTNode::T_DERIVED_COLUMN);
  EXPECT(derived->getChildren().size() == 2);
  auto tbl = derived->getChildren()[0];
  EXPECT(*tbl == ASTNode::T_COLUMN_NAME);
  EXPECT(*tbl->getToken() == Token::T_IDENTIFIER);
  EXPECT(*tbl->getToken() == "tbl");
  EXPECT(tbl->getChildren().size() == 1);
  auto col = tbl->getChildren()[0];
  EXPECT(*col->getToken() == Token::T_IDENTIFIER);
  EXPECT(*col->getToken() == "col");
  EXPECT(*derived->getChildren()[0] == ASTNode::T_COLUMN_NAME);
  EXPECT(*derived->getChildren()[0]->getToken() == Token::T_IDENTIFIER);
  EXPECT(*derived->getChildren()[0]->getToken() == "tbl");
  EXPECT(*derived->getChildren()[1] == ASTNode::T_COLUMN_ALIAS);
  EXPECT(*derived->getChildren()[1]->getToken() == Token::T_IDENTIFIER);
  EXPECT(*derived->getChildren()[1]->getToken() == "another");
  const auto& from = stmt->getChildren()[1];
  EXPECT(*from == ASTNode::T_FROM);
  return true;
}

// UNIT-SQLPARSER-017
bool test_sql_parser_SelectWithQuotedTableName(TestContext* ctx) {
  auto parser = parseTestQuery("SELECT `xxx` FROM sometable;");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(*stmt == ASTNode::T_SELECT);
  EXPECT(stmt->getChildren().size() == 2);
  const auto& sl = stmt->getChildren()[0];
  EXPECT(*sl == ASTNode::T_SELECT_LIST);
  EXPECT(sl->getChildren().size() == 1);
  const auto& derived = sl->getChildren()[0];
  EXPECT(*derived == ASTNode::T_DERIVED_COLUMN);
  EXPECT(derived->getChildren().size() == 1);
  auto col = derived->getChildren()[0];
  EXPECT(*col == ASTNode::T_COLUMN_NAME);
  EXPECT(*col->getToken() == Token::T_IDENTIFIER);
  EXPECT(*col->getToken() == "xxx");
  const auto& from = stmt->getChildren()[1];
  EXPECT_EQ(*from, ASTNode::T_FROM);
  EXPECT_EQ(from->getChildren().size(), 1);
  EXPECT_EQ(*from->getChildren()[0], ASTNode::T_TABLE_NAME);
  EXPECT_EQ(*from->getChildren()[0]->getToken(), Token::T_IDENTIFIER);
  EXPECT_EQ(from->getChildren()[0]->getToken()->getString(), "sometable");
  return true;
}

// UNIT-SQLPARSER-018
bool test_sql_parser_SelectMustBeFirstAssert(TestContext* ctx) {
  const char* err_msg = "unexpected token T_GROUP, expected one of SELECT, "
      "CREATE, INSERT, ALTER, DROP, CLUSTER, DRAW or IMPORT";

  EXPECT_EXCEPTION(err_msg, [] () {
    auto parser = parseTestQuery("GROUP BY SELECT");
  });

  return true;
}

// UNIT-SQLPARSER-019
bool test_sql_parser_WhereClause(TestContext* ctx) {
  auto parser = parseTestQuery("SELECT x FROM t WHERE a=1 AND a+1=2 OR b=3;");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(stmt->getChildren().size() == 3);
  const auto& where = stmt->getChildren()[2];
  EXPECT(*where == ASTNode::T_WHERE);
  EXPECT(where->getChildren().size() == 1);
  EXPECT(*where->getChildren()[0] == ASTNode::T_OR_EXPR);
  return true;
}

// UNIT-SQLPARSER-020
bool test_sql_parser_GroupByClause(TestContext* ctx) {
  auto parser = parseTestQuery("select count(x), y from t GROUP BY x;");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(stmt->getChildren().size() == 3);
  const auto& where = stmt->getChildren()[2];
  EXPECT(*where == ASTNode::T_GROUP_BY);
  EXPECT(where->getChildren().size() == 1);
  EXPECT(*where->getChildren()[0] == ASTNode::T_COLUMN_NAME);
  return true;
}

// UNIT-SQLPARSER-021
bool test_sql_parser_OrderByClause(TestContext* ctx) {
  auto parser = parseTestQuery("select a FROM t ORDER BY a DESC;");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(stmt->getChildren().size() == 3);
  const auto& order_by = stmt->getChildren()[2];
  EXPECT(*order_by == ASTNode::T_ORDER_BY);
  EXPECT(order_by->getChildren().size() == 1);
  EXPECT(*order_by->getChildren()[0] == ASTNode::T_SORT_SPEC);
  EXPECT(*order_by->getChildren()[0]->getToken() == Token::T_DESC);
  return true;
}

// UNIT-SQLPARSER-022
bool test_sql_parser_HavingClause(TestContext* ctx) {
  auto parser = parseTestQuery("select a FROM t HAVING 1=1;");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(stmt->getChildren().size() == 3);
  const auto& having = stmt->getChildren()[2];
  EXPECT(*having == ASTNode::T_HAVING);
  EXPECT(having->getChildren().size() == 1);
  EXPECT(*having->getChildren()[0] == ASTNode::T_EQ_EXPR);
  return true;
}

// UNIT-SQLPARSER-023
bool test_sql_parser_LimitClause(TestContext* ctx) {
  auto parser = parseTestQuery("select a FROM t LIMIT 10;");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(stmt->getChildren().size() == 3);
  const auto& limit = stmt->getChildren()[2];
  EXPECT(*limit == ASTNode::T_LIMIT);
  EXPECT(limit->getChildren().size() == 0);
  EXPECT(*limit->getToken() == "10");
  return true;
}

// UNIT-SQLPARSER-024
bool test_sql_parser_LimitOffsetClause(TestContext* ctx) {
  auto parser = parseTestQuery("select a FROM t LIMIT 10 OFFSET 23;");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(stmt->getChildren().size() == 3);
  const auto& limit = stmt->getChildren()[2];
  EXPECT(*limit == ASTNode::T_LIMIT);
  EXPECT(limit->getChildren().size() == 1);
  EXPECT(*limit->getToken() == "10");
  EXPECT(*limit->getChildren()[0] == ASTNode::T_OFFSET);
  EXPECT(*limit->getChildren()[0]->getToken() == "23");
  return true;
}

// UNIT-SQLPARSER-025
bool test_sql_parser_TokenizerEscaping(TestContext* ctx) {
  std::vector<Token> tl;
  tokenizeQuery(
      R"( SELECT fnord,sum(blah) from fubar blah.id = 'fnor\'dbar' + 123.5; )",
      &tl);

  EXPECT(tl.size() == 17);

  EXPECT(tl[0].getType() == Token::T_SELECT);
  EXPECT(tl[1].getType() == Token::T_IDENTIFIER);
  EXPECT(tl[1] == "fnord");
  EXPECT(tl[2].getType() == Token::T_COMMA);
  EXPECT(tl[3].getType() == Token::T_IDENTIFIER);
  EXPECT(tl[3] == "sum");
  EXPECT(tl[4].getType() == Token::T_LPAREN);
  EXPECT(tl[5].getType() == Token::T_IDENTIFIER);
  EXPECT(tl[5] == "blah");
  EXPECT(tl[6].getType() == Token::T_RPAREN);
  EXPECT(tl[7].getType() == Token::T_FROM);
  EXPECT(tl[8].getType() == Token::T_IDENTIFIER);
  EXPECT(tl[8] == "fubar");
  EXPECT(tl[9].getType() == Token::T_IDENTIFIER);
  EXPECT(tl[9] == "blah");
  EXPECT(tl[10].getType() == Token::T_DOT);
  EXPECT(tl[11].getType() == Token::T_IDENTIFIER);
  EXPECT(tl[11] == "id");
  EXPECT(tl[12].getType() == Token::T_EQUAL);
  EXPECT(tl[13].getType() == Token::T_STRING);
  EXPECT_EQ(tl[13].getString(), "fnor'dbar");
  EXPECT(tl[14].getType() == Token::T_PLUS);
  EXPECT(tl[15].getType() == Token::T_NUMERIC);
  EXPECT(tl[15] == "123.5");
  EXPECT(tl[16].getType() == Token::T_SEMICOLON);
  return true;
}

// UNIT-SQLPARSER-026
bool test_sql_parser_TokenizerSimple(TestContext* ctx) {
  std::vector<Token> tl;
  tokenizeQuery(
      " SELECT  fnord,sum(`blah-field`) from fubar"
      " WHERE blah.id= \"fn'o=,rdbar\" + 123;",
      &tl);

  EXPECT(tl[0].getType() == Token::T_SELECT);
  EXPECT(tl[1].getType() == Token::T_IDENTIFIER);
  EXPECT(tl[1] == "fnord");
  EXPECT(tl[2].getType() == Token::T_COMMA);
  EXPECT(tl[3].getType() == Token::T_IDENTIFIER);
  EXPECT(tl[3] == "sum");
  EXPECT(tl[4].getType() == Token::T_LPAREN);
  EXPECT(tl[5].getType() == Token::T_IDENTIFIER);
  EXPECT(tl[5] == "blah-field");
  EXPECT(tl[6].getType() == Token::T_RPAREN);
  EXPECT(tl[7].getType() == Token::T_FROM);
  EXPECT(tl[8].getType() == Token::T_IDENTIFIER);
  EXPECT(tl[8] == "fubar");
  //EXPECT(tl[9].getType() == Token::T_IDENTIFIER);
  //EXPECT(tl[9] == "blah");
  //EXPECT(tl[10].getType() == Token::T_DOT);
  //EXPECT(tl[11].getType() == Token::T_IDENTIFIER);
  //EXPECT(tl[11] == "id");
  //EXPECT(tl[12].getType() == Token::T_EQUAL);
  //EXPECT(tl[13].getType() == Token::T_STRING);
  //EXPECT(tl[13] == "fn'o=,rdbar");
  //EXPECT(tl[14].getType() == Token::T_PLUS);
  //EXPECT(tl[15].getType() == Token::T_NUMERIC);
  //EXPECT(tl[15] == "123");
  //EXPECT(tl[16].getType() == Token::T_SEMICOLON);
  return true;
}

// UNIT-SQLPARSER-027
bool test_sql_parser_TokenizerAsClause(TestContext* ctx) {
  auto parser = parseTestQuery(" SELECT fnord As blah from asd;");
  auto tl = &parser.getTokenList();
  EXPECT((*tl)[0].getType() == Token::T_SELECT);
  EXPECT((*tl)[1].getType() == Token::T_IDENTIFIER);
  EXPECT((*tl)[1] == "fnord");
  EXPECT((*tl)[2].getType() == Token::T_AS);
  EXPECT((*tl)[3].getType() == Token::T_IDENTIFIER);
  EXPECT((*tl)[3] == "blah");
  return true;
}

// UNIT-SQLPARSER-028
bool test_sql_parser_ParseMultipleSelects(TestContext* ctx) {
  auto parser = parseTestQuery(
      "SELECT "
      "  'Berlin' AS series, "
      "  temperature AS x, "
      "  temperature AS y "
      "FROM "
      "  city_temperatures "
      "WHERE city = \"Berlin\"; "
      " "
      "SELECT"
      "  'Tokyo' AS series, "
      "  temperature AS x, "
      "  temperature AS y "
      "FROM "
      "  city_temperatures "
      "WHERE city = \"Tokyo\";");

  EXPECT(parser.getStatements().size() == 2);
  return true;
}

// UNIT-SQLPARSER-029
bool test_sql_parser_ParseIfStatement(TestContext* ctx) {
  auto parser = parseTestQuery(" SELECT if(1, 2, 3) from asd;");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(*stmt == ASTNode::T_SELECT);
  EXPECT(stmt->getChildren().size() == 2);
  const auto& sl = stmt->getChildren()[0];
  EXPECT(*sl == ASTNode::T_SELECT_LIST);
  EXPECT(sl->getChildren().size() == 1);
  auto derived = sl->getChildren()[0];
  EXPECT(*derived == ASTNode::T_DERIVED_COLUMN);
  EXPECT(derived->getChildren().size() == 1);
  auto expr = derived->getChildren()[0];
  EXPECT(*expr == ASTNode::T_IF_EXPR);
  EXPECT(expr->getChildren().size() == 3);
  EXPECT(*expr->getChildren()[0] == ASTNode::T_LITERAL);
  EXPECT(*expr->getChildren()[0]->getToken() == Token::T_NUMERIC);
  EXPECT(*expr->getChildren()[0]->getToken() == "1");
  EXPECT(*expr->getChildren()[1] == ASTNode::T_LITERAL);
  EXPECT(*expr->getChildren()[1]->getToken() == Token::T_NUMERIC);
  EXPECT(*expr->getChildren()[1]->getToken() == "2");
  EXPECT(*expr->getChildren()[2] == ASTNode::T_LITERAL);
  EXPECT(*expr->getChildren()[2]->getToken() == Token::T_NUMERIC);
  EXPECT(*expr->getChildren()[2]->getToken() == "3");
  const auto& from = stmt->getChildren()[1];
  EXPECT(*from == ASTNode::T_FROM);
  return true;
}

// UNIT-SQLPARSER-030
bool test_sql_parser_CompareASTs(TestContext* ctx) {
  auto parser = parseTestQuery(R"(
      SELECT if(1, 2, 3) from asd;
      SELECT if(2, 3, 4) from asd;
      SELECT if(1, 2, 3) from asd;
  )");

  EXPECT(parser.getStatements().size() == 3);
  const auto a = parser.getStatements()[0];
  const auto b = parser.getStatements()[1];
  const auto c = parser.getStatements()[2];

  EXPECT_FALSE(a->compare(b));
  EXPECT_FALSE(b->compare(a));
  EXPECT_FALSE(b->compare(c));
  EXPECT_FALSE(c->compare(b));
  EXPECT_TRUE(a->compare(c));
  EXPECT_TRUE(c->compare(c));
  return true;
}

// UNIT-SQLPARSER-031
bool test_sql_parser_SimpleTableReference(TestContext* ctx) {
  auto parser = parseTestQuery("select x from t1;");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(stmt->getChildren().size() == 2);
  const auto& from = stmt->getChildren()[1];
  EXPECT(*from == ASTNode::T_FROM);
  EXPECT(from->getChildren().size() == 1);
  EXPECT(*from->getChildren()[0] == ASTNode::T_TABLE_NAME);
  return true;
}

// UNIT-SQLPARSER-032
bool test_sql_parser_TableReferenceWithAlias(TestContext* ctx) {
  auto parser = parseTestQuery("select x from t1 as t2;");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(stmt->getChildren().size() == 2);
  const auto& from = stmt->getChildren()[1];
  EXPECT(*from == ASTNode::T_FROM);
  EXPECT(from->getChildren().size() == 2);
  EXPECT(*from->getChildren()[0] == ASTNode::T_TABLE_NAME);
  auto alias = from->getChildren()[1];
  EXPECT(*alias == ASTNode::T_TABLE_ALIAS);
  EXPECT(*alias->getToken() == Token::T_IDENTIFIER);
  EXPECT(*alias->getToken() == "t2");
  return true;
}

// UNIT-SQLPARSER-033
bool test_sql_parser_ParseSimpleSubquery(TestContext* ctx) {
  auto parser = parseTestQuery("select t1.a from (select 123 as a, 435 as b) as t1;");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(stmt->getChildren().size() == 2);
  const auto& from = stmt->getChildren()[1];
  EXPECT(*from == ASTNode::T_FROM);
  EXPECT(from->getChildren().size() == 2);
  EXPECT(*from->getChildren()[0] == ASTNode::T_SELECT);
  auto alias = from->getChildren()[1];
  EXPECT(*alias == ASTNode::T_TABLE_ALIAS);
  EXPECT(*alias->getToken() == Token::T_IDENTIFIER);
  EXPECT(*alias->getToken() == "t1");
  return true;
}

// UNIT-SQLPARSER-034
bool test_sql_parser_CommaJoin(TestContext* ctx) {
  auto parser = parseTestQuery("select x from t1 as t2, t3 as t4, (select 123 as a) as t5;");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(stmt->getChildren().size() == 2);
  const auto& join1 = stmt->getChildren()[1];
  EXPECT(*join1 == ASTNode::T_INNER_JOIN);
  EXPECT(join1->getChildren().size() == 2);
  const auto& join2 = join1->getChildren()[0];
  EXPECT(*join2 == ASTNode::T_INNER_JOIN);
  EXPECT(join2->getChildren().size() == 2);
  return true;
}

// UNIT-SQLPARSER-035
bool test_sql_parser_InnerJoin(TestContext* ctx) {
  auto parser = parseTestQuery("select x from t1 JOIN t2 ON t1.x = t2.x JOIN (select 123 as a) as t3 JOIN t4 USING (t4.a, t4.b, t4.c);");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(stmt->getChildren().size() == 2);
  const auto& join1 = stmt->getChildren()[1];
  EXPECT(*join1 == ASTNode::T_INNER_JOIN);
  EXPECT(join1->getChildren().size() == 3);
  const auto& join2 = join1->getChildren()[0];
  EXPECT(*join2 == ASTNode::T_INNER_JOIN);
  EXPECT(join2->getChildren().size() == 2);
  const auto& join3 = join2->getChildren()[0];
  EXPECT(*join3 == ASTNode::T_INNER_JOIN);
  EXPECT(join3->getChildren().size() == 3);
  return true;
}

// UNIT-SQLPARSER-036
bool test_sql_parser_SimpleDrawStatement(TestContext* ctx) {
  auto parser = parseTestQuery("DRAW BARCHART;");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(*stmt == ASTNode::T_DRAW);
  EXPECT(stmt->getToken() != nullptr);
  EXPECT(*stmt->getToken() == Token::T_BARCHART);
  EXPECT(stmt->getChildren().size() == 0);
  return true;
}

// UNIT-SQLPARSER-037
bool test_sql_parser_DrawStatementWithAxes(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();
  auto parser = parseTestQuery("DRAW BARCHART AXIS LEFT AXIS RIGHT;");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(*stmt == ASTNode::T_DRAW);
  EXPECT(stmt->getToken() != nullptr);
  EXPECT(*stmt->getToken() == Token::T_BARCHART);
  EXPECT(stmt->getChildren().size() == 2);
  EXPECT(*stmt->getChildren()[0] == ASTNode::T_AXIS);
  EXPECT(*stmt->getChildren()[1] == ASTNode::T_AXIS);
  EXPECT(*stmt->getChildren()[0]->getToken() == Token::T_AXIS);
  EXPECT(*stmt->getChildren()[1]->getToken() == Token::T_AXIS);
  EXPECT(stmt->getChildren()[0]->getChildren().size() == 1);
  EXPECT(stmt->getChildren()[1]->getChildren().size() == 1);
  EXPECT(stmt->getChildren()[0]->getToken() != nullptr);
  EXPECT(stmt->getChildren()[1]->getToken() != nullptr);
  EXPECT(
      *stmt->getChildren()[0]->getChildren()[0] == ASTNode::T_AXIS_POSITION);
  EXPECT(
      *stmt->getChildren()[1]->getChildren()[0] == ASTNode::T_AXIS_POSITION);
  EXPECT(stmt->getChildren()[0]->getChildren()[0]->getToken() != nullptr);
  EXPECT(stmt->getChildren()[1]->getChildren()[0]->getToken() != nullptr);
  EXPECT(
      *stmt->getChildren()[0]->getChildren()[0]->getToken() == Token::T_LEFT);
  EXPECT(
      *stmt->getChildren()[1]->getChildren()[0]->getToken() == Token::T_RIGHT);
  return true;
}

// UNIT-SQLPARSER-038
bool test_sql_parser_DrawStatementWithExplicitYDomain(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();
  auto parser = parseTestQuery("DRAW BARCHART YDOMAIN 0, 100;");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(*stmt == ASTNode::T_DRAW);
  EXPECT(stmt->getToken() != nullptr);
  EXPECT(*stmt->getToken() == Token::T_BARCHART);
  EXPECT(stmt->getChildren().size() == 1);
  EXPECT(*stmt->getChildren()[0] == ASTNode::T_DOMAIN);
  EXPECT(stmt->getChildren()[0]->getChildren().size() == 1);
  EXPECT(*stmt->getChildren()[0]->getChildren()[0] == ASTNode::T_DOMAIN_SCALE);
  EXPECT(stmt->getChildren()[0]->getChildren()[0]->getChildren().size() == 2);
  return true;
}

// UNIT-SQLPARSER-039
bool test_sql_parser_DrawStatementWithScaleLogInvYDomain(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();
  auto parser = parseTestQuery(
      "DRAW BARCHART YDOMAIN 0, 100 LOGARITHMIC INVERT;");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(*stmt == ASTNode::T_DRAW);
  EXPECT(stmt->getToken() != nullptr);
  EXPECT(*stmt->getToken() == Token::T_BARCHART);
  EXPECT(stmt->getChildren().size() == 1);
  EXPECT(*stmt->getChildren()[0] == ASTNode::T_DOMAIN);
  EXPECT(stmt->getChildren().size() == 1);
  auto domain = stmt->getChildren()[0];
  EXPECT(domain->getChildren().size() == 3);
  EXPECT(*domain->getChildren()[0] == ASTNode::T_DOMAIN_SCALE);
  EXPECT(domain->getChildren()[0]->getChildren().size() == 2);
  EXPECT(*domain->getChildren()[1] == ASTNode::T_PROPERTY);
  EXPECT(domain->getChildren()[1]->getToken() != nullptr);
  EXPECT(*domain->getChildren()[1]->getToken() == Token::T_LOGARITHMIC);
  EXPECT(*domain->getChildren()[2] == ASTNode::T_PROPERTY);
  EXPECT(domain->getChildren()[2]->getToken() != nullptr);
  EXPECT(*domain->getChildren()[2]->getToken() == Token::T_INVERT);
  return true;
}

// UNIT-SQLPARSER-040
bool test_sql_parser_DrawStatementWithLogInvYDomain(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();
  auto parser = parseTestQuery(
      "DRAW BARCHART YDOMAIN LOGARITHMIC INVERT;");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(*stmt == ASTNode::T_DRAW);
  EXPECT(stmt->getToken() != nullptr);
  EXPECT(*stmt->getToken() == Token::T_BARCHART);
  EXPECT(stmt->getChildren().size() == 1);
  EXPECT(*stmt->getChildren()[0] == ASTNode::T_DOMAIN);
  EXPECT(stmt->getChildren().size() == 1);
  auto domain = stmt->getChildren()[0];
  EXPECT(domain->getChildren().size() == 2);
  EXPECT(*domain->getChildren()[0] == ASTNode::T_PROPERTY);
  EXPECT(domain->getChildren()[0]->getToken() != nullptr);
  EXPECT(*domain->getChildren()[0]->getToken() == Token::T_LOGARITHMIC);
  EXPECT(*domain->getChildren()[1] == ASTNode::T_PROPERTY);
  EXPECT(domain->getChildren()[1]->getToken() != nullptr);
  EXPECT(*domain->getChildren()[1]->getToken() == Token::T_INVERT);
  return true;
}

// UNIT-SQLPARSER-041
bool test_sql_parser_DrawStatementWithGrid(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();
  auto parser = parseTestQuery(
      "DRAW BARCHART GRID HORIZONTAL VERTICAL;");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(*stmt == ASTNode::T_DRAW);
  EXPECT(stmt->getToken() != nullptr);
  EXPECT(*stmt->getToken() == Token::T_BARCHART);
  EXPECT(stmt->getChildren().size() == 1);
  EXPECT(*stmt->getChildren()[0] == ASTNode::T_GRID);
  auto grid = stmt->getChildren()[0];
  EXPECT(grid->getChildren().size() == 2);
  EXPECT(*grid->getChildren()[0] == ASTNode::T_PROPERTY);
  EXPECT(grid->getChildren()[0]->getToken() != nullptr);
  EXPECT(*grid->getChildren()[0]->getToken() == Token::T_HORIZONTAL);
  EXPECT(*grid->getChildren()[1] == ASTNode::T_PROPERTY);
  EXPECT(grid->getChildren()[1]->getToken() != nullptr);
  EXPECT(*grid->getChildren()[1]->getToken() == Token::T_VERTICAL);
  return true;
}

// UNIT-SQLPARSER-042
bool test_sql_parser_DrawStatementWithTitle(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();
  auto parser = parseTestQuery(
      "DRAW BARCHART TITLE 'fnordtitle';");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(*stmt == ASTNode::T_DRAW);
  EXPECT(stmt->getToken() != nullptr);
  EXPECT(*stmt->getToken() == Token::T_BARCHART);
  EXPECT(stmt->getChildren().size() == 1);
  EXPECT(*stmt->getChildren()[0] == ASTNode::T_PROPERTY);
  EXPECT(stmt->getChildren()[0]->getChildren().size() == 1);
  auto title_expr = stmt->getChildren()[0]->getChildren()[0];
  auto title = runtime->evaluateConstExpression(
      txn.get(),
      title_expr).toString();
  EXPECT_EQ(title, "fnordtitle");
  return true;
}

// UNIT-SQLPARSER-043
bool test_sql_parser_DrawStatementWithSubtitle(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();
  auto parser = parseTestQuery(
      "DRAW BARCHART SUBTITLE 'fnordsubtitle';");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(*stmt == ASTNode::T_DRAW);
  EXPECT(stmt->getToken() != nullptr);
  EXPECT(*stmt->getToken() == Token::T_BARCHART);
  EXPECT(stmt->getChildren().size() == 1);
  EXPECT(*stmt->getChildren()[0] == ASTNode::T_PROPERTY);
  EXPECT(stmt->getChildren()[0]->getChildren().size() == 1);
  auto title_expr = stmt->getChildren()[0]->getChildren()[0];
  auto title = runtime->evaluateConstExpression(
      txn.get(),
      title_expr).toString();
  EXPECT_EQ(title, "fnordsubtitle");
  return true;
}

// UNIT-SQLPARSER-044
bool test_sql_parser_DrawStatementWithTitleAndSubtitle(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();
  auto parser = parseTestQuery(
      "DRAW BARCHART TITLE 'fnordtitle' SUBTITLE 'fnordsubtitle';");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(*stmt == ASTNode::T_DRAW);
  EXPECT(stmt->getToken() != nullptr);
  EXPECT(*stmt->getToken() == Token::T_BARCHART);
  EXPECT(stmt->getChildren().size() == 2);

  EXPECT(*stmt->getChildren()[0] == ASTNode::T_PROPERTY);
  EXPECT(stmt->getChildren()[0]->getChildren().size() == 1);
  auto title_expr = stmt->getChildren()[0]->getChildren()[0];
  auto title = runtime->evaluateConstExpression(
      txn.get(),
      title_expr).toString();
  EXPECT_EQ(title, "fnordtitle");

  EXPECT(*stmt->getChildren()[1] == ASTNode::T_PROPERTY);
  EXPECT(stmt->getChildren()[1]->getChildren().size() == 1);
  auto subtitle_expr = stmt->getChildren()[1]->getChildren()[0];
  auto subtitle = runtime->evaluateConstExpression(
      txn.get(),
      subtitle_expr).toString();
  EXPECT_EQ(subtitle, "fnordsubtitle");
  return true;
}

// UNIT-SQLPARSER-045
bool test_sql_parser_DrawStatementWithAxisTitle(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();
  auto parser = parseTestQuery("DRAW BARCHART AXIS LEFT TITLE 'axistitle';");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(*stmt == ASTNode::T_DRAW);
  EXPECT(stmt->getToken() != nullptr);
  EXPECT(*stmt->getToken() == Token::T_BARCHART);
  EXPECT(stmt->getChildren().size() == 1);
  EXPECT(*stmt->getChildren()[0] == ASTNode::T_AXIS);
  EXPECT(*stmt->getChildren()[0]->getToken() == Token::T_AXIS);
  EXPECT(stmt->getChildren()[0]->getChildren().size() == 2);
  EXPECT(
      *stmt->getChildren()[0]->getChildren()[0] == ASTNode::T_AXIS_POSITION);
  EXPECT(*stmt->getChildren()[0]->getChildren()[1] == ASTNode::T_PROPERTY);
  EXPECT(stmt->getChildren()[0]->getChildren()[1]->getChildren().size() == 1);
  auto title_expr = stmt->getChildren()[0]->getChildren()[1]->getChildren()[0];
  auto title = runtime->evaluateConstExpression(
      txn.get(),
      title_expr).toString();
  EXPECT_EQ(title, "axistitle");
  return true;
}

// UNIT-SQLPARSER-046
bool test_sql_parser_DrawStatementWithAxisLabelPos(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();
  auto parser = parseTestQuery("DRAW BARCHART AXIS LEFT TICKS INSIDE;");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(*stmt == ASTNode::T_DRAW);
  EXPECT(stmt->getToken() != nullptr);
  EXPECT(*stmt->getToken() == Token::T_BARCHART);
  EXPECT(stmt->getChildren().size() == 1);
  EXPECT(*stmt->getChildren()[0] == ASTNode::T_AXIS);
  EXPECT(*stmt->getChildren()[0]->getToken() == Token::T_AXIS);
  EXPECT(stmt->getChildren()[0]->getChildren().size() == 2);
  EXPECT(
      *stmt->getChildren()[0]->getChildren()[0] == ASTNode::T_AXIS_POSITION)
  auto labels = stmt->getChildren()[0]->getChildren()[1];
  EXPECT(*labels == ASTNode::T_AXIS_LABELS)
  EXPECT(labels->getChildren().size() == 1);
  EXPECT(*labels->getChildren()[0] == ASTNode::T_PROPERTY);
  EXPECT(labels->getChildren()[0]->getToken() != nullptr);
  EXPECT(*labels->getChildren()[0]->getToken() == Token::T_INSIDE);
  return true;
}

// UNIT-SQLPARSER-047
bool test_sql_parser_DrawStatementWithAxisLabelRotate(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();
  auto parser = parseTestQuery("DRAW BARCHART AXIS LEFT TICKS ROTATE 45;");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(*stmt == ASTNode::T_DRAW);
  EXPECT(stmt->getToken() != nullptr);
  EXPECT(*stmt->getToken() == Token::T_BARCHART);
  EXPECT(stmt->getChildren().size() == 1);
  EXPECT(*stmt->getChildren()[0] == ASTNode::T_AXIS);
  EXPECT(*stmt->getChildren()[0]->getToken() == Token::T_AXIS);
  EXPECT(stmt->getChildren()[0]->getChildren().size() == 2);
  EXPECT(
      *stmt->getChildren()[0]->getChildren()[0] == ASTNode::T_AXIS_POSITION)
  auto labels = stmt->getChildren()[0]->getChildren()[1];
  EXPECT(*labels == ASTNode::T_AXIS_LABELS)
  EXPECT(labels->getChildren().size() == 1);
  EXPECT(*labels->getChildren()[0] == ASTNode::T_PROPERTY);
  EXPECT(labels->getChildren()[0]->getToken() != nullptr);
  EXPECT(*labels->getChildren()[0]->getToken() == Token::T_ROTATE);
  EXPECT(labels->getChildren()[0]->getChildren().size() == 1);
  auto deg_expr = labels->getChildren()[0]->getChildren()[0];
  auto deg = runtime->evaluateConstExpression(
      txn.get(),
      deg_expr).toString();
  EXPECT_EQ(deg, "45");
  return true;
}

// UNIT-SQLPARSER-048
bool test_sql_parser_DrawStatementWithAxisLabelPosAndRotate(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();
  auto parser = parseTestQuery(
      "DRAW BARCHART AXIS LEFT TICKS OUTSIDE ROTATE 45;");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(*stmt == ASTNode::T_DRAW);
  EXPECT(stmt->getToken() != nullptr);
  EXPECT(*stmt->getToken() == Token::T_BARCHART);
  EXPECT(stmt->getChildren().size() == 1);
  EXPECT(*stmt->getChildren()[0] == ASTNode::T_AXIS);
  EXPECT(*stmt->getChildren()[0]->getToken() == Token::T_AXIS);
  EXPECT(stmt->getChildren()[0]->getChildren().size() == 2);
  EXPECT(
      *stmt->getChildren()[0]->getChildren()[0] == ASTNode::T_AXIS_POSITION)
  auto labels = stmt->getChildren()[0]->getChildren()[1];
  EXPECT(*labels == ASTNode::T_AXIS_LABELS)
  EXPECT(labels->getChildren().size() == 2);
  EXPECT(*labels->getChildren()[0] == ASTNode::T_PROPERTY);
  EXPECT(labels->getChildren()[0]->getToken() != nullptr);
  EXPECT(*labels->getChildren()[0]->getToken() == Token::T_OUTSIDE);
  EXPECT(*labels->getChildren()[1] == ASTNode::T_PROPERTY);
  EXPECT(labels->getChildren()[1]->getToken() != nullptr);
  EXPECT(*labels->getChildren()[1]->getToken() == Token::T_ROTATE);
  EXPECT(labels->getChildren()[1]->getChildren().size() == 1);
  auto deg_expr = labels->getChildren()[1]->getChildren()[0];
  auto deg = runtime->evaluateConstExpression(
      txn.get(),
      deg_expr).toString();
  EXPECT_EQ(deg, "45");
  return true;
}

// UNIT-SQLPARSER-049
bool test_sql_parser_DrawStatementWithSimpleLegend(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();
  auto parser = parseTestQuery("DRAW BARCHART LEGEND TOP LEFT INSIDE;");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(*stmt == ASTNode::T_DRAW);
  EXPECT(stmt->getToken() != nullptr);
  EXPECT(*stmt->getToken() == Token::T_BARCHART);
  EXPECT(stmt->getChildren().size() == 1);
  EXPECT(*stmt->getChildren()[0] == ASTNode::T_LEGEND);
  EXPECT(stmt->getChildren()[0]->getChildren().size() == 3);
  auto props = stmt->getChildren()[0]->getChildren();
  EXPECT(*props[0] == ASTNode::T_PROPERTY);
  EXPECT(props[0]->getToken() != nullptr);
  EXPECT(*props[0]->getToken() == Token::T_TOP);
  EXPECT(*props[1] == ASTNode::T_PROPERTY);
  EXPECT(props[1]->getToken() != nullptr);
  EXPECT(*props[1]->getToken() == Token::T_LEFT);
  EXPECT(*props[2] == ASTNode::T_PROPERTY);
  EXPECT(props[2]->getToken() != nullptr);
  EXPECT(*props[2]->getToken() == Token::T_INSIDE);
  return true;
}

// UNIT-SQLPARSER-050
bool test_sql_parser_DrawStatementWithLegendWithTitle(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();
  auto parser = parseTestQuery(
      "DRAW BARCHART LEGEND TOP LEFT INSIDE TITLE 'fnordylegend';");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(*stmt == ASTNode::T_DRAW);
  EXPECT(stmt->getToken() != nullptr);
  EXPECT(*stmt->getToken() == Token::T_BARCHART);
  EXPECT(stmt->getChildren().size() == 1);
  EXPECT(*stmt->getChildren()[0] == ASTNode::T_LEGEND);
  EXPECT(stmt->getChildren()[0]->getChildren().size() == 4);
  auto props = stmt->getChildren()[0]->getChildren();
  EXPECT(*props[0] == ASTNode::T_PROPERTY);
  EXPECT(props[0]->getToken() != nullptr);
  EXPECT(*props[0]->getToken() == Token::T_TOP);
  EXPECT(*props[1] == ASTNode::T_PROPERTY);
  EXPECT(props[1]->getToken() != nullptr);
  EXPECT(*props[1]->getToken() == Token::T_LEFT);
  EXPECT(*props[2] == ASTNode::T_PROPERTY);
  EXPECT(props[2]->getToken() != nullptr);
  EXPECT(*props[2]->getToken() == Token::T_INSIDE);
  EXPECT(props[3]->getChildren().size() == 1);
  auto title_expr = props[3]->getChildren()[0];
  auto title = runtime->evaluateConstExpression(
      txn.get(),
      title_expr).toString();
  EXPECT_EQ(title, "fnordylegend");
  return true;
}

// UNIT-SQLPARSER-051
bool test_sql_parser_CreateTableStatement(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();
  auto parser = parseTestQuery("CREATE TABLE fnord ()");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(*stmt == ASTNode::T_CREATE_TABLE);
  EXPECT(stmt->getChildren().size() == 2);
  EXPECT_EQ(*stmt->getChildren()[0], ASTNode::T_TABLE_NAME);
  EXPECT_EQ(*stmt->getChildren()[0]->getToken(), Token::T_IDENTIFIER);
  EXPECT_EQ(stmt->getChildren()[0]->getToken()->getString(), "fnord");
  EXPECT(*stmt->getChildren()[1] == ASTNode::T_COLUMN_LIST);
  return true;
}

// UNIT-SQLPARSER-052
bool test_sql_parser_CreateTableStatement2(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();
  auto parser = parseTestQuery(
      R"(
        CREATE TABLE fnord (
            time DATETIME NOT NULL Primary Key,
            myvalue STring,
            other_val REPEATED String,
            record_val RECORD (
                val1 uint64 NOT NULL,
                val2 REPEATED string
            ),
            some_other REPEATED RECORD (
                val1 uint64 NOT NULL,
                val2 REPEATED STRING
            ),
            PRIMARY KEY (time, myvalue)
        )
      )");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(*stmt == ASTNode::T_CREATE_TABLE);
  EXPECT(stmt->getChildren().size() == 2);
  EXPECT_EQ(*stmt->getChildren()[0], ASTNode::T_TABLE_NAME);
  EXPECT_EQ(*stmt->getChildren()[0]->getToken(), Token::T_IDENTIFIER);
  EXPECT_EQ(stmt->getChildren()[0]->getToken()->getString(), "fnord");
  EXPECT(*stmt->getChildren()[1] == ASTNode::T_COLUMN_LIST);
  EXPECT(stmt->getChildren()[1]->getChildren().size() == 6);
  return true;
}

// UNIT-SQLPARSER-053
bool test_sql_parser_CreateTableWithStatement(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();
  auto parser = parseTestQuery(
      R"(
        CREATE TABLE fnord (
            time DATETIME NOT NULL,
            myvalue string,
            PRIMARY KEY (time, myvalue)
        )
        WITH some_prop = "blah"
        AND other.prop = 1
      )");

  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(*stmt == ASTNode::T_CREATE_TABLE);
  EXPECT(stmt->getChildren().size() == 3);
  EXPECT_EQ(*stmt->getChildren()[0], ASTNode::T_TABLE_NAME);
  EXPECT_EQ(*stmt->getChildren()[0]->getToken(), Token::T_IDENTIFIER);
  EXPECT_EQ(stmt->getChildren()[0]->getToken()->getString(), "fnord");

  EXPECT(*stmt->getChildren()[1] == ASTNode::T_COLUMN_LIST);


  EXPECT(*stmt->getChildren()[2] == ASTNode::T_TABLE_PROPERTY_LIST);
  auto properties = stmt->getChildren()[2]->getChildren();
  EXPECT_EQ(properties.size(), 2);
  EXPECT(*properties[0] == ASTNode::T_TABLE_PROPERTY);
  EXPECT_EQ(properties[0]->getChildren().size(), 2);
  EXPECT(*properties[0]->getChildren()[0] == ASTNode::T_TABLE_PROPERTY_KEY);
  EXPECT_EQ(
      properties[0]->getChildren()[0]->getToken()->getString(),
      "some_prop");
  EXPECT(*properties[0]->getChildren()[1] == ASTNode::T_TABLE_PROPERTY_VALUE);
  EXPECT_EQ(
      properties[0]->getChildren()[1]->getToken()->getString(),
      "blah");

  EXPECT(*properties[1] == ASTNode::T_TABLE_PROPERTY);
  EXPECT_EQ(properties[1]->getChildren().size(), 2);
  EXPECT(*properties[1]->getChildren()[0] == ASTNode::T_TABLE_PROPERTY_KEY);
  EXPECT_EQ(
      properties[1]->getChildren()[0]->getToken()->getString(),
      "other.prop");
  EXPECT(*properties[1]->getChildren()[1] == ASTNode::T_TABLE_PROPERTY_VALUE);
  EXPECT_EQ(
      properties[1]->getChildren()[1]->getToken()->getString(),
      "1");
  return true;
}

// UNIT-SQLPARSER-054
bool test_sql_parser_DropTableStatement(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();
  auto parser = parseTestQuery("DROP TABLE fnord");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(*stmt == ASTNode::T_DROP_TABLE);
  EXPECT(stmt->getChildren().size() == 1);
  EXPECT_EQ(*stmt->getChildren()[0], ASTNode::T_TABLE_NAME);
  EXPECT_EQ(*stmt->getChildren()[0]->getToken(), Token::T_IDENTIFIER);
  EXPECT_EQ(stmt->getChildren()[0]->getToken()->getString(), "fnord");
  return true;
}

// UNIT-SQLPARSER-055
bool test_sql_parser_InsertIntoStatement(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  auto parser = parseTestQuery(
      R"(
          INSERT INTO evtbl (
              evtime,
              evid,
              rating,
              is_admin,
              type
          ) VALUES (
              1464463790,
              'xxx',
              1 + 2,
              true,
              null
          );
      )");

  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  const auto& children = stmt->getChildren();

  EXPECT(*stmt == ASTNode::T_INSERT_INTO);
  EXPECT(children.size() == 3);
  EXPECT_EQ(*children[0], ASTNode::T_TABLE_NAME);
  EXPECT_EQ(*children[0]->getToken(), Token::T_IDENTIFIER);
  EXPECT_EQ(children[0]->getToken()->getString(), "evtbl");

  EXPECT(*children[1] == ASTNode::T_COLUMN_LIST);
  EXPECT(children[1]->getChildren().size() == 5);
  EXPECT(*children[1]->getChildren()[0] == ASTNode::T_COLUMN_NAME);
  EXPECT_EQ(children[1]->getChildren()[0]->getToken()->getString(), "evtime");
  EXPECT_EQ(children[1]->getChildren()[1]->getToken()->getString(), "evid");

  EXPECT(*children[2] == ASTNode::T_VALUE_LIST);
  EXPECT(children[2]->getChildren().size() == 5);
  EXPECT_EQ(*children[2]->getChildren()[0], ASTNode::T_LITERAL);
  EXPECT_EQ(children[2]->getChildren()[0]->getToken()->getString(), "1464463790");
  EXPECT_EQ(children[2]->getChildren()[1]->getToken()->getString(), "xxx");

  EXPECT_EQ(*children[2]->getChildren()[2], ASTNode::T_ADD_EXPR);
  EXPECT_EQ(children[2]->getChildren()[2]->getChildren().size(), 2);
  EXPECT_EQ(*children[2]->getChildren()[2]->getChildren()[0], ASTNode::T_LITERAL);
  EXPECT_EQ(*children[2]->getChildren()[2]->getChildren()[1], ASTNode::T_LITERAL);

  EXPECT_EQ(*children[2]->getChildren()[3]->getToken(), Token::T_TRUE);
  EXPECT_EQ(*children[2]->getChildren()[4]->getToken(), Token::T_NULL);
  return true;
}

// UNIT-SQLPARSER-056
bool test_sql_parser_InsertIntoShortStatement(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  auto parser = parseTestQuery(
      R"(
          INSERT evtbl VALUES (
              1464463790,
              'xxx',
              1 + 2,
              true,
              null
          );
      )");

  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  const auto& children = stmt->getChildren();

  EXPECT(*stmt == ASTNode::T_INSERT_INTO);
  EXPECT(children.size() == 3);
  EXPECT_EQ(*children[0], ASTNode::T_TABLE_NAME);
  EXPECT_EQ(*children[0]->getToken(), Token::T_IDENTIFIER);
  EXPECT_EQ(children[0]->getToken()->getString(), "evtbl");

  EXPECT(*children[1] == ASTNode::T_COLUMN_LIST);
  EXPECT(children[1]->getChildren().size() == 0);

  EXPECT(*children[2] == ASTNode::T_VALUE_LIST);
  EXPECT(children[2]->getChildren().size() == 5);
  EXPECT_EQ(*children[2]->getChildren()[0], ASTNode::T_LITERAL);
  EXPECT_EQ(children[2]->getChildren()[0]->getToken()->getString(), "1464463790");
  EXPECT_EQ(children[2]->getChildren()[1]->getToken()->getString(), "xxx");

  EXPECT_EQ(*children[2]->getChildren()[2], ASTNode::T_ADD_EXPR);
  EXPECT_EQ(children[2]->getChildren()[2]->getChildren().size(), 2);
  EXPECT_EQ(*children[2]->getChildren()[2]->getChildren()[0], ASTNode::T_LITERAL);
  EXPECT_EQ(*children[2]->getChildren()[2]->getChildren()[1], ASTNode::T_LITERAL);

  EXPECT_EQ(*children[2]->getChildren()[3]->getToken(), Token::T_TRUE);
  EXPECT_EQ(*children[2]->getChildren()[4]->getToken(), Token::T_NULL);
  return true;
}

// UNIT-SQLPARSER-057
bool test_sql_parser_InsertIntoFromJSONStatement(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  auto parser = parseTestQuery(
      R"(
          INSERT INTO evtbl
          FROM JSON
          '{
              \"evtime\":1464463791,\"evid\":\"xxx\",
              \"products\":[
                  {\"id\":1,\"price\":1.23},
                  {\"id\":2,\"price\":3.52}
              ]
          }';
      )");

  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  const auto& children = stmt->getChildren();

  EXPECT(*stmt == ASTNode::T_INSERT_INTO);
  EXPECT_EQ(children.size(), 2);

  EXPECT_EQ(*children[0], ASTNode::T_TABLE_NAME);
  EXPECT_EQ(*children[0]->getToken(), Token::T_IDENTIFIER);
  EXPECT_EQ(children[0]->getToken()->getString(), "evtbl");

  EXPECT(*children[1] == ASTNode::T_JSON_STRING);
  return true;
}

// UNIT-SQLPARSER-058
bool test_sql_parser_CreateDatabaseStatement(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  auto parser = parseTestQuery("CREATE DATABASE events;");

  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(*stmt == ASTNode::T_CREATE_DATABASE);

  EXPECT_EQ(stmt->getChildren().size(), 1);
  EXPECT(*stmt->getChildren()[0] == ASTNode::T_DATABASE_NAME);
  EXPECT_EQ(*stmt->getChildren()[0]->getToken(), Token::T_IDENTIFIER);
  EXPECT_EQ(stmt->getChildren()[0]->getToken()->getString(), "events");
  return true;
}

// UNIT-SQLPARSER-059
bool test_sql_parser_UseDatabaseStatement(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  auto parser = parseTestQuery("USE events;");

  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(*stmt == ASTNode::T_USE_DATABASE);

  EXPECT_EQ(stmt->getChildren().size(), 1);
  EXPECT(*stmt->getChildren()[0] == ASTNode::T_DATABASE_NAME);
  EXPECT_EQ(*stmt->getChildren()[0]->getToken(), Token::T_IDENTIFIER);
  EXPECT_EQ(stmt->getChildren()[0]->getToken()->getString(), "events");
  return true;
}

// UNIT-SQLPARSER-060
bool test_sql_parser_AlterTableStatement(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  auto parser = parseTestQuery(
      R"(
          ALTER TABLE evtbl
            ADD description.id REPEATED String,
            ADD COLUMN product RECORD,
            DROP place,
            DROP column version;
      )");

  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  const auto& children = stmt->getChildren();
  EXPECT_EQ(children.size(), 5);
  EXPECT_EQ(*children[0], ASTNode::T_TABLE_NAME);
  EXPECT_EQ(children[0]->getToken()->getString(), "evtbl");

  EXPECT_EQ(*children[1], ASTNode::T_COLUMN);
  EXPECT_EQ(*children[1]->getChildren()[0], ASTNode::T_COLUMN_NAME);
  EXPECT_EQ(*children[1]->getChildren()[1], ASTNode::T_COLUMN_TYPE);
  EXPECT_EQ(children[1]->getChildren()[1]->getToken()->getString(), "String");
  EXPECT_EQ(*children[1]->getChildren()[2], ASTNode::T_REPEATED);
  EXPECT_EQ(children[1]->getChildren()[0]->getToken()->getString(), "description.id");

  EXPECT_EQ(*children[2], ASTNode::T_COLUMN);
  EXPECT_EQ(children[2]->getChildren()[0]->getToken()->getString(), "product");
  EXPECT_EQ(*children[2]->getChildren()[0], ASTNode::T_COLUMN_NAME);
  EXPECT_EQ(*children[2]->getChildren()[1], ASTNode::T_RECORD);

  EXPECT_EQ(*children[3], ASTNode::T_COLUMN_NAME);
  EXPECT_EQ(children[3]->getToken()->getString(), "place");
  EXPECT_EQ(*children[4], ASTNode::T_COLUMN_NAME);
  EXPECT_EQ(children[4]->getToken()->getString(), "version");
  return true;
}

// UNIT-SQLPARSER-061
bool test_sql_parser_AlterTableSetPropertyStatement(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  auto parser = parseTestQuery(
      R"(
          ALTER TABLE evtbl
            ADD COLUMN id STRING,
            SET PROPERTY disable_split = true,
            SET PROPERTY disable_replication = false;
      )");

  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  const auto& children = stmt->getChildren();
  EXPECT_EQ(children.size(), 4);
  EXPECT_EQ(*children[0], ASTNode::T_TABLE_NAME);
  EXPECT_EQ(children[0]->getToken()->getString(), "evtbl");

  EXPECT_EQ(*children[1], ASTNode::T_COLUMN);

  EXPECT_EQ(*children[2], ASTNode::T_TABLE_PROPERTY);
  EXPECT_EQ(children[2]->getChildren().size(), 2);
  EXPECT(*children[2]->getChildren()[0] == ASTNode::T_TABLE_PROPERTY_KEY);
  EXPECT_EQ(
      children[2]->getChildren()[0]->getToken()->getString(),
      "disable_split");
  EXPECT(*children[2]->getChildren()[1] == ASTNode::T_TABLE_PROPERTY_VALUE);
  EXPECT_EQ(
      *children[2]->getChildren()[1]->getToken(),
      Token::T_TRUE);

  EXPECT_EQ(*children[3], ASTNode::T_TABLE_PROPERTY);
  EXPECT_EQ(children[3]->getChildren().size(), 2);
  EXPECT(*children[3]->getChildren()[0] == ASTNode::T_TABLE_PROPERTY_KEY);
  EXPECT_EQ(
      children[3]->getChildren()[0]->getToken()->getString(),
      "disable_replication");
  EXPECT(*children[3]->getChildren()[1] == ASTNode::T_TABLE_PROPERTY_VALUE);
  EXPECT_EQ(
      *children[3]->getChildren()[1]->getToken(),
      Token::T_FALSE);
  return true;
}

// UNIT-SQLPARSER-062
bool test_sql_parser_DescribePartitionsStatement(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();
  auto parser = parseTestQuery("DESCRIBE PARTITIONS my_tbl;");

  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT_EQ(*stmt, ASTNode::T_DESCRIBE_PARTITIONS);
  EXPECT_EQ(stmt->getChildren().size(), 1);
  EXPECT_EQ(*stmt->getChildren()[0], ASTNode::T_TABLE_NAME);
  EXPECT_EQ(stmt->getChildren()[0]->getToken()->getString(), "my_tbl");
  return true;
}

// UNIT-SQLPARSER-063
bool test_sql_parser_DescribeServersStatement(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();
  auto parser = parseTestQuery("CLUSTER SHOW SERVERS;");

  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT_EQ(*stmt, ASTNode::T_CLUSTER_SHOW_SERVERS);
  return true;
}

// UNIT-SQLPARSER-064
bool test_sql_parser_create_table_with_partition_key(TestContext* ctx) {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();
  auto parser = parseTestQuery(
      R"(
        CREATE TABLE fnord (
            time DATETIME NOT NULL,
            myvalue string,
            other_val REPEATED string,
            PRIMARY KEY (time, myvalue),
            PARTITION KEY (time)
        )
      )");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(*stmt == ASTNode::T_CREATE_TABLE);
  EXPECT(stmt->getChildren().size() == 2);
  EXPECT_EQ(*stmt->getChildren()[0], ASTNode::T_TABLE_NAME);
  EXPECT_EQ(*stmt->getChildren()[0]->getToken(), Token::T_IDENTIFIER);
  EXPECT_EQ(stmt->getChildren()[0]->getToken()->getString(), "fnord");
  EXPECT(*stmt->getChildren()[1] == ASTNode::T_COLUMN_LIST);
  EXPECT(stmt->getChildren()[1]->getChildren().size() == 5);
  return true;
}

// UNIT-SQLPARSER-065
bool test_sql_parser_create_partition(TestContext* ctx) {
  {
    auto runtime = Runtime::getDefaultRuntime();
    auto txn = runtime->newTransaction();
    auto parser = parseTestQuery("CREATE PARTITION my_part ON test;");
    EXPECT(parser.getStatements().size() == 1);
    const auto& stmt = parser.getStatements()[0];
    EXPECT(*stmt == ASTNode::T_CREATE_PARTITION);
    EXPECT(stmt->getChildren().size() == 2);
    EXPECT_EQ(*stmt->getChildren()[0], ASTNode::T_PARTITION_NAME);
    EXPECT_EQ(*stmt->getChildren()[0]->getToken(), Token::T_IDENTIFIER);
    EXPECT_EQ(stmt->getChildren()[0]->getToken()->getString(), "my_part");
    EXPECT_EQ(*stmt->getChildren()[1], ASTNode::T_TABLE_NAME);
    EXPECT_EQ(*stmt->getChildren()[1]->getToken(), Token::T_IDENTIFIER);
    EXPECT_EQ(stmt->getChildren()[1]->getToken()->getString(), "test");
  }

  {
    auto runtime = Runtime::getDefaultRuntime();
    auto txn = runtime->newTransaction();
    auto parser = parseTestQuery("CREATE PARTITION my_part ON TABLE test;");
    EXPECT(parser.getStatements().size() == 1);
    const auto& stmt = parser.getStatements()[0];
    EXPECT(*stmt == ASTNode::T_CREATE_PARTITION);
    EXPECT(stmt->getChildren().size() == 2);
    EXPECT_EQ(*stmt->getChildren()[0], ASTNode::T_PARTITION_NAME);
    EXPECT_EQ(*stmt->getChildren()[0]->getToken(), Token::T_IDENTIFIER);
    EXPECT_EQ(stmt->getChildren()[0]->getToken()->getString(), "my_part");
    EXPECT_EQ(*stmt->getChildren()[1], ASTNode::T_TABLE_NAME);
    EXPECT_EQ(*stmt->getChildren()[1]->getToken(), Token::T_IDENTIFIER);
    EXPECT_EQ(stmt->getChildren()[1]->getToken()->getString(), "test");
  }
  return true;
}

// UNIT-SQLPARSER-066
bool test_sql_parser_drop_partition(TestContext* ctx) {
  {
    auto runtime = Runtime::getDefaultRuntime();
    auto txn = runtime->newTransaction();
    auto parser = parseTestQuery("DROP PARTITION my_part FROM test;");
    EXPECT(parser.getStatements().size() == 1);
    const auto& stmt = parser.getStatements()[0];
    EXPECT(*stmt == ASTNode::T_DROP_PARTITION);
    EXPECT(stmt->getChildren().size() == 2);
    EXPECT_EQ(*stmt->getChildren()[0], ASTNode::T_PARTITION_NAME);
    EXPECT_EQ(*stmt->getChildren()[0]->getToken(), Token::T_IDENTIFIER);
    EXPECT_EQ(stmt->getChildren()[0]->getToken()->getString(), "my_part");
    EXPECT_EQ(*stmt->getChildren()[1], ASTNode::T_TABLE_NAME);
    EXPECT_EQ(*stmt->getChildren()[1]->getToken(), Token::T_IDENTIFIER);
    EXPECT_EQ(stmt->getChildren()[1]->getToken()->getString(), "test");
  }

  {
    auto runtime = Runtime::getDefaultRuntime();
    auto txn = runtime->newTransaction();
    auto parser = parseTestQuery("DROP PARTITION my_part FROM TABLE test;");
    EXPECT(parser.getStatements().size() == 1);
    const auto& stmt = parser.getStatements()[0];
    EXPECT(*stmt == ASTNode::T_DROP_PARTITION);
    EXPECT(stmt->getChildren().size() == 2);
    EXPECT_EQ(*stmt->getChildren()[0], ASTNode::T_PARTITION_NAME);
    EXPECT_EQ(*stmt->getChildren()[0]->getToken(), Token::T_IDENTIFIER);
    EXPECT_EQ(stmt->getChildren()[0]->getToken()->getString(), "my_part");
    EXPECT_EQ(*stmt->getChildren()[1], ASTNode::T_TABLE_NAME);
    EXPECT_EQ(*stmt->getChildren()[1]->getToken(), Token::T_IDENTIFIER);
    EXPECT_EQ(stmt->getChildren()[1]->getToken()->getString(), "test");
  }
  return true;
}

static const std::vector<std::string> kTestQueries = {
  // UNIT-SQLPARSER-QUERY-001
  "SELECT -sum(fnord) + (123 * 4);",

  // UNIT-SQLPARSER-QUERY-002
  "SELECT (-blah + sum(fnord) / (123 * 4)) as myfield;",

  // UNIT-SQLPARSER-QUERY-003
  "SELECT concat(fnord + 5, -somefunc(myotherfield)) + (123 * 4);",

  // UNIT-SQLPARSER-QUERY-004
  "  SELECT"
  "     l_orderkey,"
  "     sum( l_extendedprice * ( 1 - l_discount) ) AS revenue,"
  "     o_orderdate,"
  "     o_shippriority"
  "  FROM"
  "     customer,"
  "     orders,"
  "     lineitem "
  "  WHERE"
  "    c_mktsegment = 'FURNITURE' AND"
  "    c_custkey = o_custkey AND"
  "    l_orderkey = o_orderkey AND"
  "    o_orderdate < \"2013-12-21\" AND"
  "    l_shipdate > \"2014-01-06\""
  "  GROUP BY"
  "    l_orderkey,"
  "    o_orderdate,"
  "    o_shippriority"
  "  ORDER BY"
  "    revenue,"
  "    o_orderdate;",

  // UNIT-SQLPARSER-QUERY-005
  R"(select testtable.time from testtable;)",

  // UNIT-SQLPARSER-QUERY-006
  R"(select t1.time from testtable t1;)",

  // UNIT-SQLPARSER-QUERY-007
  R"(select count(1) from testtable;)",

  // UNIT-SQLPARSER-QUERY-008
  R"(select count(event.search_query.time) from testtable;)",

  // UNIT-SQLPARSER-QUERY-009
  R"(select sum(event.search_query.num_result_items) from testtable;)",

  // UNIT-SQLPARSER-QUERY-010
  R"(select sum(count(event.search_query.result_items.position) WITHIN RECORD) from testtable;)",

  // UNIT-SQLPARSER-QUERY-011
  R"(
    select
      sum(event.search_query.num_result_items) WITHIN RECORD,
      count(event.search_query.result_items.position) WITHIN RECORD
    from testtable;)",

  // UNIT-SQLPARSER-QUERY-012
  R"(
    select
      1,
      event.search_query.time,
      event.search_query.num_result_items,
      event.search_query.result_items.position
    from testtable;)",

  // UNIT-SQLPARSER-QUERY-013
  R"(
    select
      count(time),
      sum(count(event.search_query.time) WITHIN RECORD),
      sum(sum(event.search_query.num_result_items) WITHIN RECORD),
      sum(count(event.search_query.result_items.position) WITHIN RECORD)
    from testtable;)",

  // UNIT-SQLPARSER-QUERY-014
  R"(
    select
      count(time),
      sum(count(event.search_query.time) WITHIN RECORD),
      sum(sum(event.search_query.num_result_items) WITHIN RECORD),
      sum(count(event.search_query.result_items.position) WITHIN RECORD),
      (
        count(time) +
        sum(count(event.search_query.time) WITHIN RECORD) +
        sum(sum(event.search_query.num_result_items) WITHIN RECORD) +
        sum(count(event.search_query.result_items.position) WITHIN RECORD)
      )
    from testtable
    group by 1;)",

  // UNIT-SQLPARSER-QUERY-015
  R"(
    select
      count(1) as num_items,
      sum(if(s.c, 1, 0)) as clicks
    from (
        select
            event.search_query.result_items.position as p,
            event.search_query.result_items.clicked as c
        from testtable) as s
        where s.p = 6;)",

  // UNIT-SQLPARSER-QUERY-016
  R"(select fnord from 'test.tbl';)",

  // UNIT-SQLPARSER-QUERY-017
  R"(select * from testtable order by time desc limit 10;)",

  // UNIT-SQLPARSER-QUERY-018
  R"(select * from testtable;)",

  // UNIT-SQLPARSER-QUERY-019
  R"(select value, time from testtable;)",

  // UNIT-SQLPARSER-QUERY-020
  R"(select * from (select value, time from testtable);)",

  // UNIT-SQLPARSER-QUERY-021
  R"(select * from (select * from (select value, time from testtable));)",

  // UNIT-SQLPARSER-QUERY-022
  R"(select * from (select * from (select * from testtable));)",

  // UNIT-SQLPARSER-QUERY-023
  R"(select 123 as a, 435 as b;)",

  // UNIT-SQLPARSER-QUERY-024
  R"(select t1.b, a from (select 123 as a, 435 as b) as t1)",

  // UNIT-SQLPARSER-QUERY-025
  R"(select * from (select 123 as a, 435 as b) as t1)",

  // UNIT-SQLPARSER-QUERY-026
  R"(select count(1), t1.fubar + t1.x from (select count(1) as x, 123 as fubar from testtable group by TRUNCATE(time / 2000000)) t1 GROUP BY t1.x;)",

  // UNIT-SQLPARSER-QUERY-027
  R"(select t1.x from (select count(1) as x from testtable group by TRUNCATE(time / 2000000)) t1  order by t1.x DESC LIMIT 2;)",

  // UNIT-SQLPARSER-QUERY-028
  R"(select * from testtable group by time;)",

  // UNIT-SQLPARSER-QUERY-029
  R"(
    SELECT
      t1.time, t2.time, t3.time, t1.x, t2.x, t1.x + t2.x, t1.x * 3 = t3.x, x1, x2, x3
    FROM
      (select TRUNCATE(time / 1000000) as time, count(1) as x, 123 as x1 from testtable group by TRUNCATE(time / 1200000000)) t1,
      (select TRUNCATE(time / 1000000) as time, sum(2) as x, 456 as x2 from testtable group by TRUNCATE(time / 1200000000)) AS t2,
      (select TRUNCATE(time / 1000000) as time, sum(3) as x, 789 as x3 from testtable group by TRUNCATE(time / 1200000000)) AS t3
    ORDER BY
      t1.time desc;)",

  // UNIT-SQLPARSER-QUERY-030
  R"(
    SELECT
      t1.time, t2.time, t3.time, t1.x, t2.x, t1.x + t2.x, t1.x * 3 = t3.x, x1, x2, x3
    FROM
      (select TRUNCATE(time / 1000000) as time, count(1) as x, 123 as x1 from testtable group by TRUNCATE(time / 1200000000)) t1
    JOIN
      (select TRUNCATE(time / 1000000) as time, sum(2) as x, 456 as x2 from testtable group by TRUNCATE(time / 1200000000)) AS t2
    JOIN
      (select TRUNCATE(time / 1000000) as time, sum(3) as x, 789 as x3 from testtable group by TRUNCATE(time / 1200000000)) AS t3
    ON
      t2.time = t1.time and t3.time = t2.time
    ORDER BY
      t1.time desc;)",

  // UNIT-SQLPARSER-QUERY-031
  R"(
    SELECT
      t1.time, t2.time, t3.time, t1.x, t2.x, t1.x + t2.x, t1.x * 3 = t3.x, x1, x2, x3
    FROM
      (select TRUNCATE(time / 1000000) as time, count(1) as x, 123 as x1 from testtable group by TRUNCATE(time / 1200000000)) t1
    JOIN
      (select TRUNCATE(time / 1000000) as time, sum(2) as x, 456 as x2 from testtable group by TRUNCATE(time / 1200000000)) AS t2
    JOIN
      (select TRUNCATE(time / 1000000) as time, sum(3) as x, 789 as x3 from testtable group by TRUNCATE(time / 1200000000)) AS t3
    WHERE
      t2.time = t1.time AND t1.time = t3.time
    ORDER BY
        t1.time desc;)",

  // UNIT-SQLPARSER-QUERY-032
  R"(
    SELECT customers.customername, orders.orderid
    FROM customers
    LEFT JOIN orders
    ON customers.customerid=orders.customerid
    ORDER BY customers.customername;)",

  // UNIT-SQLPARSER-QUERY-033
  R"(
    SELECT customers.customername, orders.orderid
    FROM customers
    LEFT JOIN orders
    ON customers.customerid=orders.customerid
    WHERE customers.country = 'UK'
    ORDER BY customers.customername;)",

  // UNIT-SQLPARSER-QUERY-034
  R"(
    SELECT orders.orderid, employees.firstname
    FROM orders
    RIGHT JOIN employees
    ON orders.employeeid=employees.employeeid
    ORDER BY orders.orderid;)",

  // UNIT-SQLPARSER-QUERY-035
  R"(
    SELECT orders.orderid, employees.firstname
    FROM orders
    RIGHT JOIN employees
    ON orders.employeeid=employees.employeeid
    WHERE employees.firstname = 'Steven'
    ORDER BY orders.orderid;)",

  // UNIT-SQLPARSER-QUERY-036
  R"(
    SELECT *
    FROM departments
    JOIN users
    ON users.deptid = departments.deptid
    ORDER BY name;)",

  // UNIT-SQLPARSER-QUERY-037
  R"(
    SELECT *
    FROM departments, users, openinghours
    WHERE users.deptid = departments.deptid AND openinghours.deptid = departments.deptid
    ORDER BY name;)",

  // UNIT-SQLPARSER-QUERY-038
  R"(
    SELECT * FROM (
      SELECT *
      FROM departments, users, openinghours
      WHERE users.deptid = departments.deptid AND openinghours.deptid = departments.deptid
    ) ORDER BY name;)",

  // UNIT-SQLPARSER-QUERY-039
  R"(
    SELECT *
    FROM departments
    NATURAL JOIN users
    ORDER BY name;)",


  // UNIT-SQLPARSER-QUERY-040
  R"(
    SELECT *
    FROM departments
    NATURAL JOIN openinghours
    NATURAL JOIN users
    ORDER BY name;)",

  // UNIT-SQLPARSER-QUERY-041
  R"(
    SELECT *
    FROM (SELECT * FROM departments) t1
    NATURAL JOIN (SELECT deptid, start_time, end_time FROM openinghours) t2
    NATURAL JOIN (SELECT * FROM users) t3
    ORDER BY name;)",


  // UNIT-SQLPARSER-QUERY-042
  R"(
    select
      FROM_TIMESTAMP(TRUNCATE(time / 86400000000) *  86400),
      sum(sum(if(event.cart_items.checkout_step = 1, event.cart_items.price_cents, 0)) WITHIN RECORD) / 100.0 as gmv_eur,
      sum(if(event.cart_items.checkout_step = 1, event.cart_items.price_cents, 0)) / sum(count(event.page_view.item_id) WITHIN RECORD) as fu
      from testtable
      group by TRUNCATE(time / 86400000000)
        order by time desc;)",


  // UNIT-SQLPARSER-QUERY-043
  R"(select count(1) cnt, time from testtable group by TRUNCATE(time / 60000000) order by cnt desc;)",

  // UNIT-SQLPARSER-QUERY-044
  R"(select time from testtable group by TRUNCATE(time / 60000000);)",

  // UNIT-SQLPARSER-QUERY-045
  R"(select user_id from testtable order by time desc limit 10;)",

  // UNIT-SQLPARSER-QUERY-046
  R"(SELECT 'fubar' REGEX '^b' as x;)",

  // UNIT-SQLPARSER-QUERY-047
  R"(
    SELECT customername
    FROM customers
    ORDER BY customername;)",

  // UNIT-SQLPARSER-QUERY-048
  R"(show tables;)",

  // UNIT-SQLPARSER-QUERY-049
  R"(describe departments;)",

  // UNIT-SQLPARSER-QUERY-050
  R"(select now();)",

  // UNIT-SQLPARSER-QUERY-051
  R"(
    SELECT *
    FROM departments
    JOIN users
    ORDER BY name
    LIMIT 5;)",

  // UNIT-SQLPARSER-QUERY-052
  "  DRAW LINECHART AXIS LEFT;"
  ""
  "  SELECT"
  "    'series1' as series, one AS x, two AS y"
  "  FROM"
  "    testtable;"
  ""
  "  SELECT"
  "    'series2' as series, one as x, two + 5 as y"
  "  from"
  "    testtable;"
  ""
  "  SELECT"
  "    'series3' as series, one as x, two / 2 + 4 as y"
  "  FROM"
  "    testtable;",

  // UNIT-SQLPARSER-QUERY-053
  "  DRAW BARCHART"
  "    ORIENTATION HORIZONTAL"
  "    AXIS LEFT"
  "    AXIS BOTTOM"
  "    LEGEND TOP RIGHT INSIDE TITLE 'gdp per country';"
  ""
  "  SELECT"
  "    'gross domestic product per country' as series,"
  "    country as x,"
  "    gbp as y"
  "  FROM"
  "    gdp_per_country"
  "  LIMIT 30;",

  // UNIT-SQLPARSER-QUERY-054
  R"(
    select sum(value), count(value), min(value), max(value) FROM testtable;)"

};

bool test_sql_parser_query(TestContext* ctx, const std::string& query) {
  auto parser = parseTestQuery(query.c_str());
  EXPECT(parser.getStatements().size() > 0);
  return true;
}

void setup_unit_sql_parser_tests(TestRepository* repo) {
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-001", sql_parser, SimpleValueExpression);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-002", sql_parser, ArithmeticValueExpression);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-003", sql_parser, ArithmeticValueExpressionParens);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-004", sql_parser, ParseEqual);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-005", sql_parser, ParseNotEqual);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-006", sql_parser, ParseGreaterThan);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-007", sql_parser, ParseGreaterThanEqual);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-008", sql_parser, ArithmeticValueExpressionPrecedence);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-009", sql_parser, MethodCallValueExpression);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-010", sql_parser, NegatedValueExpression);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-011", sql_parser, SelectAll);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-012", sql_parser, SelectTableWildcard);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-013", sql_parser, QuotedTableName);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-014", sql_parser, NestedTableName);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-015", sql_parser, SelectDerivedColumn);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-016", sql_parser, SelectDerivedColumnWithTableName);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-017", sql_parser, SelectWithQuotedTableName);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-018", sql_parser, SelectMustBeFirstAssert);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-019", sql_parser, WhereClause);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-020", sql_parser, GroupByClause);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-021", sql_parser, OrderByClause);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-022", sql_parser, HavingClause);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-023", sql_parser, LimitClause);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-024", sql_parser, LimitOffsetClause);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-025", sql_parser, TokenizerEscaping);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-026", sql_parser, TokenizerSimple);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-027", sql_parser, TokenizerAsClause);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-028", sql_parser, ParseMultipleSelects);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-029", sql_parser, ParseIfStatement);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-030", sql_parser, CompareASTs);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-031", sql_parser, SimpleTableReference);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-032", sql_parser, TableReferenceWithAlias);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-033", sql_parser, ParseSimpleSubquery);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-034", sql_parser, CommaJoin);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-035", sql_parser, InnerJoin);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-036", sql_parser, SimpleDrawStatement);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-037", sql_parser, DrawStatementWithAxes);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-038", sql_parser, DrawStatementWithExplicitYDomain);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-039", sql_parser, DrawStatementWithScaleLogInvYDomain);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-040", sql_parser, DrawStatementWithLogInvYDomain);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-041", sql_parser, DrawStatementWithGrid);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-042", sql_parser, DrawStatementWithTitle);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-043", sql_parser, DrawStatementWithSubtitle);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-044", sql_parser, DrawStatementWithTitleAndSubtitle);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-045", sql_parser, DrawStatementWithAxisTitle);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-046", sql_parser, DrawStatementWithAxisLabelPos);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-047", sql_parser, DrawStatementWithAxisLabelRotate);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-048", sql_parser, DrawStatementWithAxisLabelPosAndRotate);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-049", sql_parser, DrawStatementWithSimpleLegend);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-050", sql_parser, DrawStatementWithLegendWithTitle);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-051", sql_parser, CreateTableStatement);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-052", sql_parser, CreateTableStatement2);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-053", sql_parser, CreateTableWithStatement);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-054", sql_parser, DropTableStatement);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-055", sql_parser, InsertIntoStatement);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-056", sql_parser, InsertIntoShortStatement);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-057", sql_parser, InsertIntoFromJSONStatement);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-058", sql_parser, CreateDatabaseStatement);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-059", sql_parser, UseDatabaseStatement);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-060", sql_parser, AlterTableStatement);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-061", sql_parser, AlterTableSetPropertyStatement);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-062", sql_parser, DescribePartitionsStatement);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-063", sql_parser, DescribeServersStatement);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-064", sql_parser, create_table_with_partition_key);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-065", sql_parser, create_partition);
  SETUP_UNIT_TESTCASE(repo, "UNIT-SQLPARSER-066", sql_parser, drop_partition);

  for (size_t i = 0; i < kTestQueries.size(); ++i) {
    std::stringstream test_id;
    test_id << "UNIT-SQLPARSER-QUERY-" << std::setfill('0') << std::setw(3) << i + 1;

    TestCase test_case;
    test_case.test_id = test_id.str();
    test_case.description = StringUtil::format("SQL parser test query #$0", i + 1);
    test_case.suites = std::set<TestSuite> { TestSuite::WORLD, TestSuite::SMOKE };
    test_case.fun =  std::bind(
        &test_sql_parser_query,
        std::placeholders::_1,
        kTestQueries[i]);

    repo->addTestBundle({ test_case });
  }
}

} // namespace unit
} // namespace test
} // namespace eventql

