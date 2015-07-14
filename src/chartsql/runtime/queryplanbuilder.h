/**
 * This file is part of the "FnordMetric" project
 *   Copyright (c) 2014 Paul Asmuth, Google Inc.
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <fnord/stdtypes.h>
#include <fnord/autoref.h>
#include <chartsql/parser/token.h>
#include <chartsql/parser/astnode.h>
#include <chartsql/runtime/queryplan.h>
#include <chartsql/runtime/compile.h>

namespace csql {
class QueryPlanNode;
class TableRepository;
class Runtime;

class QueryPlanBuilder {
public:

  QueryPlanBuilder(SymbolTable* symbol_table);

  RefPtr<QueryTreeNode> build(ASTNode* ast, RefPtr<TableProvider> tables);

  Vector<RefPtr<QueryTreeNode>> build(
      const Vector<ASTNode*>& ast,
      RefPtr<TableProvider> tables);

  ValueExpressionNode* buildValueExpression(ASTNode* ast);

protected:

  /**
   * Returns true if the ast is a SELECT statement that has columns in its
   * select list that are not of the form T_TABLE_NAME -> T_COLUMN_NAME
   */
  bool hasUnexpandedColumns(ASTNode* ast) const;

  /**
   * Returns true if the ast is a SELECT statement that has a join
   */
  bool hasJoin(ASTNode* ast) const;

  /**
   * Returns true if the ast is a SELECT statement that has a GROUP BY clause,
   * otherwise false
   */
  bool hasGroupByClause(ASTNode* ast) const;

  ///**
  // * Returns true if the ast is a SELECT statement that has a GROUP OVER
  // * TIMEWINDOW clause, otherwise false
  // */
  //bool hasGroupOverTimewindowClause(ASTNode* ast) const;

  /**
   * Returns true if the ast is a SELECT statement that has a ORDER BY clause,
   * otherwise false
   */
  bool hasOrderByClause(ASTNode* ast) const;

  /**
   * Returns true if the ast is a SELECT statement with a select list that
   * contains at least one aggregation expression, otherwise false.
   */
  bool hasAggregationInSelectList(ASTNode* ast) const;

  /**
   * Walks the ast recursively and returns true if at least one aggregation
   * expression was found, otherwise false.
   */
  bool hasAggregationExpression(ASTNode* ast) const;

  /**
   * Walks the ast recursively and returns true if at least one aggregation
   * WITHIN RECORD expression was found, otherwise false.
   */
  bool hasAggregationWithinRecord(ASTNode* ast) const;

  /**
   * Build a group by query plan node for a SELECT statement that has a GROUP
   * BY clause
   */
  QueryTreeNode* buildGroupBy(ASTNode* ast, RefPtr<TableProvider> tables);

  ///**
  // * Build a group over timewindow query plan node for a SELECT statement that
  // * has a GROUP OVer TIMEWINDOW clause
  // */
  //QueryPlanNode* buildGroupOverTimewindow(ASTNode* ast, TableRepository* repo);


  QueryTreeNode* buildSequentialScan(ASTNode* ast);

  QueryTreeNode* buildLimitClause(ASTNode* ast, RefPtr<TableProvider> tables);

  QueryTreeNode* buildOrderByClause(ASTNode* ast, RefPtr<TableProvider> tables);

  /**
   * Builds a standalone SELECT expression (A SELECT without any tables)
   */
  QueryTreeNode* buildSelectExpression(ASTNode* ast);

  QueryTreeNode* buildShowTables(ASTNode* ast);

  QueryTreeNode* buildDescribeTable(ASTNode* ast);

  ValueExpressionNode* buildOperator(const std::string& name, ASTNode* ast);

  ValueExpressionNode* buildLiteral(ASTNode* ast);

  ValueExpressionNode* buildColumnReference(ASTNode* ast);

  ValueExpressionNode* buildIfStatement(ASTNode* ast);

  ValueExpressionNode* buildMethodCall(ASTNode* ast);

  /**
   * expand all column names + wildcard to tablename->columnanme
   */
  void expandColumns(ASTNode* ast, RefPtr<TableProvider> tables);

  /**
   * Recursively walk the provided ast and search for column references. For
   * each found column reference, add the column reference to the provided
   * select list and replace the original column reference with an index into
   * the new select list.
   *
   * This is used to create child select lists for nested query plan nodes.
   */
  bool buildInternalSelectList(ASTNode* ast, ASTNode* select_list);

  SelectListNode* buildSelectList(ASTNode* select_list);


  SymbolTable* symbol_table_;
};

}
