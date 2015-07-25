/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stx/stdtypes.h>
#include <stx/exception.h>
#include <stx/wallclock.h>
#include <stx/test/unittest.h>
#include <chartsql/parser/parser.h>
#include <chartsql/parser/token.h>
#include <chartsql/parser/tokenize.h>

using namespace stx;
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
  EXPECT(*tbl == ASTNode::T_TABLE_NAME);
  EXPECT(*tbl->getToken() == Token::T_IDENTIFIER);
  EXPECT(*tbl->getToken() == "tbl");
  EXPECT(tbl->getChildren().size() == 1);
  auto col = tbl->getChildren()[0];
  EXPECT(*col->getToken() == Token::T_IDENTIFIER);
  EXPECT(*col->getToken() == "col");
  EXPECT(*derived->getChildren()[0] == ASTNode::T_TABLE_NAME);
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

TEST_CASE(ParserTest, TestFromList, [] () {
  auto parser = parseTestQuery("SELECT a FROM tbl1, tbl2;");
  EXPECT(parser.getStatements().size() == 1);
  const auto& stmt = parser.getStatements()[0];
  const auto& from = stmt->getChildren()[1];
  EXPECT(*from == ASTNode::T_FROM);
  EXPECT(from->getChildren().size() == 2);
  EXPECT(*from->getChildren()[0] == ASTNode::T_TABLE_NAME);
  EXPECT(*from->getChildren()[0]->getToken() == "tbl1");
  EXPECT(*from->getChildren()[1] == ASTNode::T_TABLE_NAME);
  EXPECT(*from->getChildren()[1]->getToken() == "tbl2");
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
      " SELECT  fnord,sum(blah) from fubar blah.id"
      "= 'fnor\\'dbar' + 123.5;",
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
  //EXPECT(tl[13] == "fnord'bar"); // FIXPAUL
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
    new stx::test::UnitTest::TestCase(
        &ParserTest,
        "TestComplexQueries",
        [query] () {
          auto parser = parseTestQuery(query);
          EXPECT(parser.getStatements().size() == 1);
        });
  }
});
