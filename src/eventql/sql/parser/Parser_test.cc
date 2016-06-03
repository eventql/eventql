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
#include <eventql/util/stdtypes.h>
#include <eventql/util/exception.h>
#include <eventql/util/wallclock.h>
#include <eventql/util/test/unittest.h>
#include <eventql/sql/parser/parser.h>
#include <eventql/sql/parser/token.h>
#include <eventql/sql/parser/tokenize.h>
#include "eventql/sql/runtime/defaultruntime.h"

#include "eventql/eventql.h"
using namespace csql;

UNIT_TEST(ParserTest);

static Parser parseTestQuery(const char* query) {
  Parser parser;
  parser.parse(query, strlen(query));
  return parser;
}

TEST_CASE(ParserTest, TestSimpleValueExpression, [] () {
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
});

TEST_CASE(ParserTest, TestArithmeticValueExpression, [] () {
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
});

TEST_CASE(ParserTest, TestArithmeticValueExpressionParens, [] () {
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
});

TEST_CASE(ParserTest, TestParseEqual, [] () {
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
});

TEST_CASE(ParserTest, TestParseNotEqual, [] () {
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
});

TEST_CASE(ParserTest, ParseGreaterThan, [] () {
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
});

TEST_CASE(ParserTest, ParseGreaterThanEqual, [] () {
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
});

TEST_CASE(ParserTest, TestArithmeticValueExpressionPrecedence, [] () {
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
});

TEST_CASE(ParserTest, TestMethodCallValueExpression, [] () {
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
});

TEST_CASE(ParserTest, TestNegatedValueExpression, [] () {
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
});

TEST_CASE(ParserTest, TestSelectAll, [] () {
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
});

TEST_CASE(ParserTest, TestSelectTableWildcard, [] () {
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
});

TEST_CASE(ParserTest, TestQuotedTableName, [] () {
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
});

TEST_CASE(ParserTest, TestNestedTableName, [] () {
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
});

TEST_CASE(ParserTest, TestSelectDerivedColumn, [] () {
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
});

TEST_CASE(ParserTest, TestSelectDerivedColumnWithTableName, [] () {
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
});

TEST_CASE(ParserTest, TestSelectWithQuotedTableName, [] () {
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
});

TEST_CASE(ParserTest, TestSelectMustBeFirstAssert, [] () {
  const char* err_msg = "unexpected token T_GROUP, expected one of SELECT, "
      "DRAW or IMPORT";

  EXPECT_EXCEPTION(err_msg, [] () {
    auto parser = parseTestQuery("GROUP BY SELECT");
  });
});

TEST_CASE(ParserTest, TestWhereClause, [] () {
  auto parser = parseTestQuery("SELECT x FROM t WHERE a=1 AND a+1=2 OR b=3;");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(stmt->getChildren().size() == 3);
  const auto& where = stmt->getChildren()[2];
  EXPECT(*where == ASTNode::T_WHERE);
  EXPECT(where->getChildren().size() == 1);
  EXPECT(*where->getChildren()[0] == ASTNode::T_OR_EXPR);
});

TEST_CASE(ParserTest, TestGroupByClause, [] () {
  auto parser = parseTestQuery("select count(x), y from t GROUP BY x;");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(stmt->getChildren().size() == 3);
  const auto& where = stmt->getChildren()[2];
  EXPECT(*where == ASTNode::T_GROUP_BY);
  EXPECT(where->getChildren().size() == 1);
  EXPECT(*where->getChildren()[0] == ASTNode::T_COLUMN_NAME);
});

TEST_CASE(ParserTest, TestOrderByClause, [] () {
  auto parser = parseTestQuery("select a FROM t ORDER BY a DESC;");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(stmt->getChildren().size() == 3);
  const auto& order_by = stmt->getChildren()[2];
  EXPECT(*order_by == ASTNode::T_ORDER_BY);
  EXPECT(order_by->getChildren().size() == 1);
  EXPECT(*order_by->getChildren()[0] == ASTNode::T_SORT_SPEC);
  EXPECT(*order_by->getChildren()[0]->getToken() == Token::T_DESC);
});

TEST_CASE(ParserTest, TestHavingClause, [] () {
  auto parser = parseTestQuery("select a FROM t HAVING 1=1;");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(stmt->getChildren().size() == 3);
  const auto& having = stmt->getChildren()[2];
  EXPECT(*having == ASTNode::T_HAVING);
  EXPECT(having->getChildren().size() == 1);
  EXPECT(*having->getChildren()[0] == ASTNode::T_EQ_EXPR);
});

TEST_CASE(ParserTest, TestLimitClause, [] () {
  auto parser = parseTestQuery("select a FROM t LIMIT 10;");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(stmt->getChildren().size() == 3);
  const auto& limit = stmt->getChildren()[2];
  EXPECT(*limit == ASTNode::T_LIMIT);
  EXPECT(limit->getChildren().size() == 0);
  EXPECT(*limit->getToken() == "10");
});

TEST_CASE(ParserTest, TestLimitOffsetClause, [] () {
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
});

TEST_CASE(ParserTest, TestTokenizerEscaping, [] () {
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
});

TEST_CASE(ParserTest, TestTokenizerSimple, [] () {
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
});

TEST_CASE(ParserTest, TestTokenizerAsClause, [] () {
  auto parser = parseTestQuery(" SELECT fnord As blah from asd;");
  auto tl = &parser.getTokenList();
  EXPECT((*tl)[0].getType() == Token::T_SELECT);
  EXPECT((*tl)[1].getType() == Token::T_IDENTIFIER);
  EXPECT((*tl)[1] == "fnord");
  EXPECT((*tl)[2].getType() == Token::T_AS);
  EXPECT((*tl)[3].getType() == Token::T_IDENTIFIER);
  EXPECT((*tl)[3] == "blah");
});

TEST_CASE(ParserTest, TestParseMultipleSelects, [] () {
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
});

TEST_INITIALIZER(ParserTest, InitializeComplexQueries, [] () {
  std::vector<const char*> queries;
  queries.push_back("SELECT -sum(fnord) + (123 * 4);");
  queries.push_back("SELECT (-blah + sum(fnord) / (123 * 4)) as myfield;");
  queries.push_back(
      "SELECT concat(fnord + 5, -somefunc(myotherfield)) + (123 * 4);");
  queries.push_back(
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
        "    o_orderdate;");

  for (auto query : queries) {
    new test::UnitTest::TestCase(
        &ParserTest,
        "TestComplexQueries",
        [query] () {
          auto parser = parseTestQuery(query);
          EXPECT(parser.getStatements().size() == 1);
        });
  }
});

TEST_CASE(ParserTest, TestParseIfStatement, [] () {
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
});

TEST_CASE(ParserTest, TestCompareASTs, [] () {
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
});

TEST_CASE(ParserTest, TestSimpleTableReference, [] () {
  auto parser = parseTestQuery("select x from t1;");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(stmt->getChildren().size() == 2);
  const auto& from = stmt->getChildren()[1];
  EXPECT(*from == ASTNode::T_FROM);
  EXPECT(from->getChildren().size() == 1);
  EXPECT(*from->getChildren()[0] == ASTNode::T_TABLE_NAME);
});

TEST_CASE(ParserTest, TestTableReferenceWithAlias, [] () {
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
});

TEST_CASE(ParserTest, TestParseSimpleSubquery, [] () {
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
});

TEST_CASE(ParserTest, TestCommaJoin, [] () {
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
});

TEST_CASE(ParserTest, TestInnerJoin, [] () {
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
});

TEST_CASE(ParserTest, TestSimpleDrawStatement, [] () {
  auto parser = parseTestQuery("DRAW BARCHART;");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  EXPECT(*stmt == ASTNode::T_DRAW);
  EXPECT(stmt->getToken() != nullptr);
  EXPECT(*stmt->getToken() == Token::T_BARCHART);
  EXPECT(stmt->getChildren().size() == 0);
});

TEST_CASE(ParserTest, TestDrawStatementWithAxes, [] () {
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
});

TEST_CASE(ParserTest, TestDrawStatementWithExplicitYDomain, [] () {
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
});

TEST_CASE(ParserTest, TestDrawStatementWithScaleLogInvYDomain, [] () {
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
});

TEST_CASE(ParserTest, TestDrawStatementWithLogInvYDomain, [] () {
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
});

TEST_CASE(ParserTest, TestDrawStatementWithGrid, [] () {
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
});

TEST_CASE(ParserTest, TestDrawStatementWithTitle, [] () {
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
  EXPECT_EQ(title.getString(), "fnordtitle");
});

TEST_CASE(ParserTest, TestDrawStatementWithSubtitle, [] () {
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
  EXPECT_EQ(title.getString(), "fnordsubtitle");
});

TEST_CASE(ParserTest, TestDrawStatementWithTitleAndSubtitle, [] () {
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
  EXPECT_EQ(title.getString(), "fnordtitle");

  EXPECT(*stmt->getChildren()[1] == ASTNode::T_PROPERTY);
  EXPECT(stmt->getChildren()[1]->getChildren().size() == 1);
  auto subtitle_expr = stmt->getChildren()[1]->getChildren()[0];
  auto subtitle = runtime->evaluateConstExpression(
      txn.get(),
      subtitle_expr).toString();
  EXPECT_EQ(subtitle.getString(), "fnordsubtitle");
});

TEST_CASE(ParserTest, TestDrawStatementWithAxisTitle, [] () {
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
  EXPECT_EQ(title.getString(), "axistitle");
});

TEST_CASE(ParserTest, TestDrawStatementWithAxisLabelPos, [] () {
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
});

TEST_CASE(ParserTest, TestDrawStatementWithAxisLabelRotate, [] () {
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
  EXPECT_EQ(deg.getString(), "45");
});

TEST_CASE(ParserTest, TestDrawStatementWithAxisLabelPosAndRotate, [] () {
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
  EXPECT_EQ(deg.getString(), "45");
});

TEST_CASE(ParserTest, TestDrawStatementWithSimpleLegend, [] () {
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
});

TEST_CASE(ParserTest, TestDrawStatementWithLegendWithTitle, [] () {
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
  EXPECT_EQ(title.getString(), "fnordylegend");
});

TEST_CASE(ParserTest, TestCreateTableStatement, [] () {
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
});

TEST_CASE(ParserTest, TestCreateTableStatement2, [] () {
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
});

TEST_CASE(ParserTest, TestInsertIntoStatement, [] () {
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
});

TEST_CASE(ParserTest, TestInsertIntoFromJSONStatement, [] () {
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
});

TEST_CASE(ParserTest, TestInsertIntoSetStatement, [] () {
  auto runtime = Runtime::getDefaultRuntime();
  auto txn = runtime->newTransaction();

  auto parser = parseTestQuery(
      R"(
          INSERT INTO evtbl SET
              evtime = 1464463790,
              evid = 'xxx',
              rating = 1 + 2,
              is_admin = true,
              type = null
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
});
