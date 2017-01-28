/**
 * Copyright (c) 2016 DeepCortex GmbH <legal@eventql.io>
 * Authors:
 *   - Paul Asmuth <paul@eventql.io>
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
#pragma once
#include <eventql/util/stdtypes.h>
#include <eventql/util/autoref.h>
#include <eventql/sql/parser/token.h>
#include <eventql/sql/parser/astnode.h>
#include <eventql/sql/query_plan.h>
#include <eventql/sql/runtime/compiler.h>

namespace csql {
class QueryPlanNode;
class TableRepository;
class Runtime;
class SelectListNode;

struct QueryPlanBuilderOptions {
  QueryPlanBuilderOptions() :
      enable_constant_folding(true) {}

  bool enable_constant_folding;
};


class QueryPlanBuilder : public RefCounted {
public:

  using ColumnResolver = Function<std::pair<size_t, SType> (const String&)>;
  static const ColumnResolver kEmptyColumnResolver;

  using ColumnTypeResolver = Function<SType (size_t)>;
  static const ColumnTypeResolver kEmptyColumnTypeResolver;

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
      ASTNode* ast,
      ColumnResolver resolver = kEmptyColumnResolver,
      ColumnTypeResolver type_resolver = kEmptyColumnTypeResolver);

  RefPtr<ValueExpressionNode> buildUnoptimizedValueExpression(
      Transaction* txn,
      ASTNode* ast,
      ColumnResolver resolver,
      ColumnTypeResolver type_resolver);

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

  QueryTreeNode* buildDescribePartitions(
      Transaction* txn,
      ASTNode* ast);

  QueryTreeNode* buildClusterShowServers(
      Transaction* txn,
      ASTNode* ast);

  QueryTreeNode* buildCreateTable(
      Transaction* txn,
      ASTNode* ast);

  QueryTreeNode* buildCreateDatabase(
      Transaction* txn,
      ASTNode* ast);

  QueryTreeNode* buildUseDatabase(
      Transaction* txn,
      ASTNode* ast);

  QueryTreeNode* buildDropTable(
      Transaction* txn,
      ASTNode* ast);

  QueryTreeNode* buildInsertInto(
      Transaction* txn,
      ASTNode* ast);

  QueryTreeNode* buildAlterTable(
      Transaction* txn,
      ASTNode* ast);

  ValueExpressionNode* buildOperator(
      Transaction* txn,
      const std::string& name,
      ASTNode* ast,
      ColumnResolver resolver,
      ColumnTypeResolver type_resolver);

  ValueExpressionNode* buildLiteral(
      Transaction* txn,
      ASTNode* ast);

  ValueExpressionNode* buildColumnReference(
      Transaction* txn,
      ASTNode* ast,
      ColumnResolver resolver);

  ValueExpressionNode* buildColumnIndex(
      Transaction* txn,
      ASTNode* ast,
      ColumnTypeResolver type_resolver);

  ValueExpressionNode* buildIfStatement(
      Transaction* txn,
      ASTNode* ast,
      ColumnResolver resolver,
      ColumnTypeResolver type_resolver);

  ValueExpressionNode* buildMethodCall(
      Transaction* txn,
      ASTNode* ast,
      ColumnResolver resolver,
      ColumnTypeResolver type_resolver);

  ValueExpressionNode* buildRegex(
      Transaction* txn,
      ASTNode* ast,
      ColumnResolver resolver,
      ColumnTypeResolver type_resolver);

  ValueExpressionNode* buildLike(
      Transaction* txn,
      ASTNode* ast,
      ColumnResolver resolver,
      ColumnTypeResolver type_resolver);

  /**
   * assign explicit column names to all output columns
   */
  void assignExplicitColumnNames(
      Transaction* txn,
      ASTNode* ast,
      RefPtr<TableProvider> tables);

  SelectListNode* buildSelectList(
      Transaction* txn,
      ASTNode* select_list,
      ColumnResolver resolver,
      ColumnTypeResolver type_resolver);

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
