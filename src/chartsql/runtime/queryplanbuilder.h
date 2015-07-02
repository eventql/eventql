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

/**
 * All QueryPlanBuilder imeplementations must be thread safe. Specifically they
 * must support calling the buildQueryPlan method concurrenctly from many
 * threads
 */
class QueryPlanBuilderInterface {
public:

  QueryPlanBuilderInterface(
      ValueExpressionBuilder* compiler,
      const std::vector<std::unique_ptr<Backend>>& backends) :
      compiler_(compiler),
      backends_(backends) {}

  virtual ~QueryPlanBuilderInterface() {}

  virtual QueryPlanNode* buildQueryPlan(
      ASTNode* statement,
      TableRepository* repo) = 0;

protected:
  ValueExpressionBuilder* compiler_;
  const std::vector<std::unique_ptr<Backend>>& backends_;
};

class QueryPlanBuilder {
public:

  QueryPlanBuilder(SymbolTable* symbol_table);

  RefPtr<QueryTreeNode> build(ASTNode* ast);

//  QueryPlanBuilder(
//      ValueExpressionBuilder* compiler,
//      const std::vector<std::unique_ptr<Backend>>& backends);
//
//  void buildQueryPlan(
//      const std::vector<std::unique_ptr<ASTNode>>& statements,
//      QueryPlan* query_plan);
//
//  QueryPlanNode* buildQueryPlan(
//      ASTNode* statement,
//      TableRepository* repo) override;
//
  void extend(std::unique_ptr<QueryPlanBuilderInterface> other);

protected:

  /**
   * Returns true if the ast is a SELECT statement that has columns in its
   * select list that are not of the form T_TABLE_NAME -> T_COLUMN_NAME
   */
  //bool hasUnexpandedColumns(ASTNode* ast) const;

  ///**
  // * Returns true if the ast is a SELECT statement that has a join
  // */
  //bool hasJoin(ASTNode* ast) const;

  ///**
  // * Returns true if the ast is a SELECT statement that has a GROUP BY clause,
  // * otherwise false
  // */
  bool hasGroupByClause(ASTNode* ast) const;

  ///**
  // * Returns true if the ast is a SELECT statement that has a GROUP OVER
  // * TIMEWINDOW clause, otherwise false
  // */
  //bool hasGroupOverTimewindowClause(ASTNode* ast) const;

  ///**
  // * Returns true if the ast is a SELECT statement that has a ORDER BY clause,
  // * otherwise false
  // */
  //bool hasOrderByClause(ASTNode* ast) const;

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

  ///**
  // * Build a group by query plan node for a SELECT statement that has a GROUP
  // * BY clause
  // */
  //void expandColumns(ASTNode* ast, TableRepository* repo);

  ///**
  // * Build a group by query plan node for a SELECT statement that has a GROUP
  // * BY clause
  // */
  QueryTreeNode* buildGroupBy(ASTNode* ast);

  ///**
  // * Build a group over timewindow query plan node for a SELECT statement that
  // * has a GROUP OVer TIMEWINDOW clause
  // */
  //QueryPlanNode* buildGroupOverTimewindow(ASTNode* ast, TableRepository* repo);

  /**
   * Recursively walk the provided ast and search for column references. For
   * each found column reference, add the column reference to the provided
   * select list and replace the original column reference with an index into
   * the new select list.
   *
   * This is used to create child select lists for nested query plan nodes.
   */
  bool buildInternalSelectList(ASTNode* ast, ASTNode* select_list);

  QueryTreeNode* buildSequentialScan(ASTNode* ast);

  QueryTreeNode* buildLimitClause(ASTNode* ast);

  /**
   * Builds a standalone SELECT expression (A SELECT without any tables)
   */
  QueryTreeNode* buildSelectExpression(ASTNode* ast);

  ValueExpressionNode* buildValueExpression(ASTNode* ast);

  SelectListNode* buildSelectList(ASTNode* select_list);

  ValueExpressionNode* buildOperator(const std::string& name, ASTNode* ast);

  ValueExpressionNode* buildLiteral(ASTNode* ast);

  ValueExpressionNode* buildColumnReference(ASTNode* ast);

  ValueExpressionNode* buildIfStatement(ASTNode* ast);

  ValueExpressionNode* buildMethodCall(ASTNode* ast);
  //QueryPlanNode* buildOrderByClause(ASTNode* ast, TableRepository* repo);

  std::vector<std::unique_ptr<QueryPlanBuilderInterface>> extensions_;

  SymbolTable* symbol_table_;

};

}
