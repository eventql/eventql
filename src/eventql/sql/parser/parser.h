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
#ifndef _FNORDMETRIC_SQL_PARSER_H
#define _FNORDMETRIC_SQL_PARSER_H
#include <stdlib.h>
#include <string>
#include <vector>
#include <memory>
#include <eventql/util/exception.h>
#include <eventql/sql/parser/token.h>
#include <eventql/sql/parser/astnode.h>

namespace csql {

/**
 * todo: subqueries, limit/offset, join, union
 */

class Parser {
  friend class QueryTest;
public:
  struct ParseError : public Exception {
    template <typename... T>
    ParseError(
        const char* message, T... args) :
        Exception(message, args...) {}
  };

  /* precedence table
    14, !
    13, - (unary minus), ~ (unary bit inversion)
    12, ^
    11, *, /, DIV, %, MOD
    10, -, +
    9, <<, >>
    8, &
    7, |
    // not implemented below here
    6, = (comparison), <=>, >=, >, <=, <, <>, !=, IS, LIKE, REGEXP, IN
    5, BETWEEN, CASE, WHEN, THEN, ELSE
    4, NOT
    3, &&, AND
    2, XOR
    1, ||, OR
  */

  Parser();

  /**
   * Parse a SQL query string and return one AST tree for every statement
   */
  std::vector<std::unique_ptr<ASTNode>> parseQuery(const std::string query);

  /**
   * Parse a SQL Query from the input string. Returns true on success and false
   * if an error occurred.
   */
  size_t parse(const char* query, size_t len);

  /**
   * Parse a single SQL scalar/value expression from the input string. Returns
   * true on success and false if an error occurred.
   */
  size_t parseValueExpression(const char* query, size_t len);

  const std::vector<ASTNode*>& getStatements() const;
  const std::vector<Token>& getTokenList() const;

  void debugPrint() const;

protected:

  ASTNode* expr(int precedence = 0);
  ASTNode* unaryExpr();
  ASTNode* columnName();
  ASTNode* binaryExpr(ASTNode* lhs, int precedence);
  ASTNode* methodCall();

  ASTNode* statement();
  ASTNode* selectStatement();
  ASTNode* selectSublist();
  ASTNode* createStatement();
  ASTNode* createTableStatement();
  ASTNode* columnDefinition();
  ASTNode* primaryKeyDefinition();
  ASTNode* tableName();
  ASTNode* insertStatement();
  ASTNode* insertIntoStatement();
  ASTNode* insertColumnList();
  ASTNode* insertValueList();
  ASTNode* insertFromJSON();
  ASTNode* alterStatement();
  ASTNode* nestedColumnName();
  ASTNode* addColumnDefinition();
  ASTNode* drawStatement();
  ASTNode* axisClause();
  ASTNode* domainClause();
  ASTNode* legendClause();

  ASTNode* importStatement();
  ASTNode* showStatement();
  ASTNode* explainStatement();
  ASTNode* explainQueryStatement();
  ASTNode* describeTableStatement();

  ASTNode* fromClause();
  ASTNode* tableReference();
  ASTNode* tableFactor();
  ASTNode* joinExpression(ASTNode* base);
  ASTNode* joinCondition();

  ASTNode* whereClause();
  ASTNode* groupByClause();
  ASTNode* groupOverClause();
  ASTNode* havingClause();
  ASTNode* orderByClause();
  ASTNode* limitClause();

  ASTNode* eqExpr(ASTNode* lhs, int precedence);
  ASTNode* neqExpr(ASTNode* lhs, int precedence);
  ASTNode* ltExpr(ASTNode* lhs, int precedence);
  ASTNode* lteExpr(ASTNode* lhs, int precedence);
  ASTNode* gtExpr(ASTNode* lhs, int precedence);
  ASTNode* gteExpr(ASTNode* lhs, int precedence);
  ASTNode* andExpr(ASTNode* lhs, int precedence);
  ASTNode* orExpr(ASTNode* lhs, int precedence);

  ASTNode* addExpr(ASTNode* lhs, int precedence);
  ASTNode* subExpr(ASTNode* lhs, int precedence);
  ASTNode* mulExpr(ASTNode* lhs, int precedence);
  ASTNode* divExpr(ASTNode* lhs, int precedence);
  ASTNode* modExpr(ASTNode* lhs, int precedence);
  ASTNode* powExpr(ASTNode* lhs, int precedence);

  ASTNode* likeExpr(ASTNode* lhs, int precedence);
  ASTNode* regexExpr(ASTNode* lhs, int precedence);

  bool assertExpectation(Token::kTokenType expectation);

  inline Token* consumeToken() {
    auto token = cur_token_;
    cur_token_++;
    return token;
  }

  inline bool consumeIf(Token::kTokenType expectation) {
    if (*cur_token_ == expectation) {
      consumeToken();
      return true;
    } else {
      return false;
    }
  }

  inline Token* expectAndConsume(Token::kTokenType expectation) {
    if (assertExpectation(expectation)) {
      return consumeToken();
    }

    return nullptr;
  }

  inline Token* expectAndConsume(
      const std::vector<Token::kTokenType>& expectation) {
    for (const auto& candidate : expectation) {
      if (*cur_token_ == candidate) {
        return consumeToken();
      }
    }

    std::string legal_tokens;
    for (const auto& candidate : expectation) {
      if (candidate != *expectation.begin()) {
        legal_tokens += ", ";
      }

      legal_tokens += Token::getTypeName(candidate);
    }

    RAISE(
        kParseError,
        "unexpected token %s%s%s, expected one of: %s",
        Token::getTypeName(cur_token_->getType()),
        cur_token_->getString().size() > 0 ? ": " : "",
        cur_token_->getString().c_str(),
        legal_tokens.c_str());

    return nullptr;
  }

  inline ASTNode* expectAndConsumeValueExpr() {
    auto val_expr = expr();

    if (val_expr == nullptr) {
      RAISE(
          kParseError,
          "unexpected token %s%s%s, expected: value expression",
          Token::getTypeName(cur_token_->getType()),
          cur_token_->getString().size() > 0 ? ": " : "",
          cur_token_->getString().c_str());
    }

    return val_expr;
  }

  inline bool lookahead(size_t n, Token::kTokenType expectation) const {
    return cur_token_ + n < token_list_end_ &&
        cur_token_[n].getType() == expectation;
  }

  std::vector<Token> token_list_;
  Token* cur_token_;
  Token* token_list_end_;
  ASTNode root_;
};

}
#endif
