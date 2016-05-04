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
#include <stx/stdtypes.h>
#include <stx/autoref.h>
#include <csql/parser/token.h>
#include <csql/parser/astnode.h>
#include <csql/runtime/queryplan.h>
#include <csql/runtime/compiler.h>

namespace csql {
class QueryPlanNode;
class TableRepository;
class Runtime;

struct QueryPlanBuilderOptions {
  QueryPlanBuilderOptions() :
      enable_constant_folding(true) {}

  bool enable_constant_folding;
};

class QueryPlanBuilder : public RefCounted {
public:

  QueryPlanBuilder(
      QueryPlanBuilderOptions opts,
      SymbolTable* symbol_table);

  RefPtr<QueryTreeNode> build(
      Transaction* txn,
      ASTNode* ast,
      RefPtr<TableProvider> tables);

  Vector<RefPtr<QueryTreeNode>> build(
      Transaction* txn,
      const Vector<ASTNode*>& ast,
      RefPtr<TableProvider> tables);

  RefPtr<ValueExpressionNode> buildValueExpression(
      Transaction* txn,
      ASTNode* ast);

  RefPtr<ValueExpressionNode> buildUnoptimizedValueExpression(
      Transaction* txn,
      ASTNode* ast);


  /**
   * Returns true if the ast is a SELECT statement that has columns in its
   * select list that are not explicitly named (expr AS name).
   */
  bool hasImplicitlyNamedColumns(ASTNode* ast) const;

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
  QueryTreeNode* buildGroupBy(
      Transaction* txn,
      ASTNode* ast,
      RefPtr<TableProvider> tables);

  QueryTreeNode* buildJoin(
      Transaction* txn,
      ASTNode* ast,
      RefPtr<TableProvider> tables);

  QueryTreeNode* buildSubquery(
      Transaction* txn,
      ASTNode* ast,
      RefPtr<TableProvider> tables);

  QueryTreeNode* buildSequentialScan(
      Transaction* txn,
      ASTNode* ast,
      RefPtr<TableProvider> tables);

  QueryTreeNode* buildLimitClause(
      Transaction* txn,
      ASTNode* ast,
      RefPtr<TableProvider> tables);

  QueryTreeNode* buildOrderByClause(
      Transaction* txn,
      ASTNode* ast,
      RefPtr<TableProvider> tables);

  /**
   * Builds a standalone SELECT expression (A SELECT without any tables)
   */
  QueryTreeNode* buildSelectExpression(
      Transaction* txn,
      ASTNode* ast);

  QueryTreeNode* buildTableReference(
      Transaction* txn,
      ASTNode* table_ref,
      ASTNode* select_list,
      ASTNode* where_clause,
      RefPtr<TableProvider> tables,
      bool in_join);

  QueryTreeNode* buildJoinTableReference(
      Transaction* txn,
      ASTNode* table_ref,
      ASTNode* select_list,
      ASTNode* where_clause,
      RefPtr<TableProvider> tables,
      bool in_join);

  QueryTreeNode* buildSubqueryTableReference(
      Transaction* txn,
      ASTNode* table_ref,
      ASTNode* select_list,
      ASTNode* where_clause,
      RefPtr<TableProvider> tables,
      bool in_join);

  QueryTreeNode* buildSeqscanTableReference(
      Transaction* txn,
      ASTNode* table_ref,
      ASTNode* select_list,
      ASTNode* where_clause,
      RefPtr<TableProvider> tables,
      bool in_join);

  QueryTreeNode* buildShowTables(
      Transaction* txn,
      ASTNode* ast);

  QueryTreeNode* buildDescribeTable(
      Transaction* txn,
      ASTNode* ast);

  ValueExpressionNode* buildOperator(
      Transaction* txn,
      const std::string& name,
      ASTNode* ast);

  ValueExpressionNode* buildLiteral(
      Transaction* txn,
      ASTNode* ast);

  ValueExpressionNode* buildColumnReference(
      Transaction* txn,
      ASTNode* ast);

  ValueExpressionNode* buildColumnIndex(
      Transaction* txn,
      ASTNode* ast);

  ValueExpressionNode* buildIfStatement(
      Transaction* txn,
      ASTNode* ast);

  ValueExpressionNode* buildMethodCall(
      Transaction* txn,
      ASTNode* ast);

  ValueExpressionNode* buildRegex(
      Transaction* txn,
      ASTNode* ast);

  ValueExpressionNode* buildLike(
      Transaction* txn,
      ASTNode* ast);

  /**
   * assign explicit column names to all output columns
   */
  void assignExplicitColumnNames(
      Transaction* txn,
      ASTNode* ast,
      RefPtr<TableProvider> tables);

  SelectListNode* buildSelectList(
      Transaction* txn,
      ASTNode* select_list);

  /**
   * Recursively walk the provided ast and search for column references. For
   * each found column reference, add the column reference to the provided
   * select list and replace the original column reference with an index into
   * the new select list.
   *
   * This is used to create child select lists for nested query plan nodes.
   */
  bool buildGroupBySelectList(
      ASTNode* ast,
      ASTNode* select_list);

protected:
  QueryPlanBuilderOptions opts_;
  SymbolTable* symbol_table_;
};

}
