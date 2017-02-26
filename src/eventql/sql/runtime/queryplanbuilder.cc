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
#include <algorithm>
#include <stdlib.h>
#include <eventql/sql/parser/astnode.h>
#include <eventql/sql/parser/astutil.h>
#include <eventql/sql/parser/parser.h>
#include <eventql/sql/runtime/queryplanbuilder.h>
#include <eventql/sql/qtree/GroupByNode.h>
#include <eventql/sql/qtree/IfExpressionNode.h>
#include <eventql/sql/qtree/SelectExpressionNode.h>
#include <eventql/sql/qtree/LimitNode.h>
#include <eventql/sql/qtree/OrderByNode.h>
#include <eventql/sql/qtree/DrawStatementNode.h>
#include <eventql/sql/qtree/ChartStatementNode.h>
#include <eventql/sql/qtree/ShowTablesNode.h>
#include <eventql/sql/qtree/DescribeTableNode.h>
#include <eventql/sql/qtree/RegexExpressionNode.h>
#include <eventql/sql/qtree/LikeExpressionNode.h>
#include <eventql/sql/qtree/SubqueryNode.h>
#include <eventql/sql/qtree/QueryTreeUtil.h>
#include <eventql/sql/qtree/ValueExpressionNode.h>
#include <eventql/sql/qtree/JoinNode.h>
#include <eventql/sql/qtree/nodes/create_database.h>
#include <eventql/sql/qtree/nodes/use_database.h>
#include <eventql/sql/qtree/nodes/alter_table.h>
#include <eventql/sql/qtree/nodes/create_table.h>
#include <eventql/sql/qtree/nodes/drop_table.h>
#include <eventql/sql/qtree/nodes/insert_into.h>
#include <eventql/sql/qtree/nodes/insert_json.h>
#include <eventql/sql/qtree/nodes/describe_partitions.h>
#include "eventql/sql/qtree/nodes/cluster_show_servers.h"
#include <eventql/sql/table_schema.h>

namespace csql {

const QueryPlanBuilder::ColumnResolver QueryPlanBuilder::kEmptyColumnResolver =
    [] (const std::string& c) { return std::make_pair(size_t(-1), SType::NIL); };

const QueryPlanBuilder::ColumnTypeResolver QueryPlanBuilder::kEmptyColumnTypeResolver =
    [] (size_t idx) -> SType { RAISE(kRuntimeError, "invald column index"); };

QueryPlanBuilder::QueryPlanBuilder(
    QueryPlanBuilderOptions opts,
    SymbolTable* symbol_table) :
    symbol_table_(symbol_table) {}

RefPtr<QueryTreeNode> QueryPlanBuilder::build(
    Transaction* txn,
    ASTNode* ast,
    RefPtr<TableProvider> tables) {
  QueryTreeNode* node = nullptr;

  /* assign explicit column names to all output columns */
  if (hasImplicitlyNamedColumns(ast)) {
    assignExplicitColumnNames(txn, ast, tables);
  }

  /* internal nodes: multi table query (joins), order, aggregation, limit */
  if ((node = buildLimitClause(txn, ast, tables)) != nullptr) {
    return node;
  }

  if (hasOrderByClause(ast)) {
    return buildOrderByClause(txn, ast, tables);
  }

  if (hasGroupByClause(ast) || hasAggregationInSelectList(ast)) {
    return buildGroupBy(txn, ast, tables);
  }

  /* leaf nodes: table scan, join, subquery tableless select */
  if ((node = buildJoin(txn, ast, tables)) != nullptr) {
    return node;
  }

  if ((node = buildSubquery(txn, ast, tables)) != nullptr) {
    return node;
  }

  if ((node = buildSequentialScan(txn, ast, tables)) != nullptr) {
    return node;
  }

  if ((node = buildSelectExpression(txn, ast)) != nullptr) {
    return node;
  }

  /* other statments */
  if ((node = buildShowTables(txn, ast)) != nullptr) {
    return node;
  }

  if ((node = buildDescribeTable(txn, ast)) != nullptr) {
    return node;
  }

  if ((node = buildDescribePartitions(txn, ast)) != nullptr) {
    return node;
  }

  if ((node = buildClusterShowServers(txn, ast)) != nullptr) {
    return node;
  }

  if ((node = buildCreateTable(txn, ast)) != nullptr) {
    return node;
  }

  if ((node = buildDropTable(txn, ast)) != nullptr) {
    return node;
  }

  if ((node = buildInsertInto(txn, ast)) != nullptr) {
    return node;
  }

  if ((node = buildCreateDatabase(txn, ast)) != nullptr) {
    return node;
  }

  if ((node = buildUseDatabase(txn, ast)) != nullptr) {
    return node;
  }

  if ((node = buildAlterTable(txn, ast)) != nullptr) {
    return node;
  }

  ast->debugPrint(2);
  RAISE(kRuntimeError, "can't figure out a query plan for this, sorry :(");
}

Vector<RefPtr<QueryTreeNode>> QueryPlanBuilder::build(
    Transaction* txn,
    const Vector<ASTNode*>& statements,
    RefPtr<TableProvider> tables) {
  Vector<RefPtr<QueryTreeNode>> nodes;

  for (int i = 0; i < statements.size(); ++i) {
    switch (statements[i]->getType()) {

      case ASTNode::T_SELECT:
      case ASTNode::T_SELECT_DEEP:
      case ASTNode::T_SHOW_TABLES:
      case ASTNode::T_DESCRIBE_TABLE:
      case ASTNode::T_DESCRIBE_PARTITIONS:
      case ASTNode::T_CLUSTER_SHOW_SERVERS:
      case ASTNode::T_CREATE_TABLE:
      case ASTNode::T_CREATE_DATABASE:
      case ASTNode::T_USE_DATABASE:
      case ASTNode::T_DROP_TABLE:
      case ASTNode::T_INSERT_INTO:
      case ASTNode::T_ALTER_TABLE:
        nodes.emplace_back(build(txn, statements[i], tables));
        break;

      case ASTNode::T_DRAW: {
        Vector<RefPtr<QueryTreeNode>> draw_nodes;

        while (i < statements.size() &&
            statements[i]->getType() == ASTNode::T_DRAW) {
          ScopedPtr<ASTNode> draw_ast(statements[i]->deepCopy());
          Vector<RefPtr<QueryTreeNode>> subselects;

          for (++i; i < statements.size(); ) {
            switch (statements[i]->getType()) {
              case ASTNode::T_SELECT:
              case ASTNode::T_SELECT_DEEP:
                subselects.emplace_back(build(txn, statements[i++], tables));
                continue;
              case ASTNode::T_DRAW:
                break;
              default:
                RAISE(
                    kRuntimeError,
                    "DRAW statments may only be followed by SELECT or END DRAW " \
                    "statements");
            }

            break;
          }

          draw_nodes.emplace_back(
              new DrawStatementNode(std::move(draw_ast), subselects));
        }

        nodes.emplace_back(new ChartStatementNode(draw_nodes));
        break;
      }

      default:
        statements[i]->debugPrint();
        RAISE(kRuntimeError, "invalid statement");
    }
  }

  return nodes;
}

//QueryPlanBuilder::QueryPlanBuilder(
//    ValueExpressionBuilder* compiler,
//    const std::vector<std::unique_ptr<Backend>>& backends) :
//    QueryPlanBuilderInterface(compiler, backends) {}

//void QueryPlanBuilder::buildQueryPlan(
//    const std::vector<std::unique_ptr<ASTNode>>& statements,
//    QueryPlan* query_plan) {
//
//  for (const auto& stmt : statements) {
//    switch (stmt->getType()) {
//      case csql::ASTNode::T_SELECT: {
//        auto query_plan_node = buildQueryPlan(
//            stmt.get(),
//            query_plan->tableRepository());
//
//        if (query_plan_node == nullptr) {
//          RAISE(
//              kRuntimeError,
//              "can't figure out a query plan for this, sorry :(");
//        }
//
//        query_plan->addQuery(std::unique_ptr<QueryPlanNode>(query_plan_node));
//        break;
//      }
//
//      case csql::ASTNode::T_IMPORT:
//        query_plan->tableRepository()->import(
//            ImportStatement(stmt.get(), compiler_),
//            backends_);
//        break;
//
//      default:
//        RAISE(kRuntimeError, "invalid statement");
//    }
//  }
//}
//
//QueryPlanNode* QueryPlanBuilder::buildQueryPlan(
//    ASTNode* ast,
//    TableRepository* repo) {
//  QueryPlanNode* exec = nullptr;
///  // if verbose -> dump ast
//  return nullptr;
//}
//

static SType resolveColumnType(const TableExpressionNode* node, size_t idx) {
  return node->getColumnType(idx);
}

bool QueryPlanBuilder::hasImplicitlyNamedColumns(ASTNode* ast) const {
  if (ast->getType() != ASTNode::T_SELECT &&
      ast->getType() != ASTNode::T_SELECT_DEEP) {
    return false;
  }

  if (ast->getChildren().size() < 1 ||
      ast->getChildren()[0]->getType() != ASTNode::T_SELECT_LIST) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  if (ast->getChildren().size() == 1) {
    return false;
  }

  for (const auto& col : ast->getChildren()[0]->getChildren()) {
    if (col->getType() == ASTNode::T_DERIVED_COLUMN &&
        col->getChildren().size() == 1) {
      return true;
    }
  }

  return false;
}

bool QueryPlanBuilder::hasGroupByClause(ASTNode* ast) const {
  if (!(*ast == ASTNode::T_SELECT) || ast->getChildren().size() < 2) {
    return false;
  }

  for (const auto& child : ast->getChildren()) {
    if (child->getType() == ASTNode::T_GROUP_BY) {
      return true;
    }
  }

  return false;
}
//
//bool QueryPlanBuilder::hasGroupOverTimewindowClause(ASTNode* ast) const {
//  if (!(*ast == ASTNode::T_SELECT) || ast->getChildren().size() < 2) {
//    return false;
//  }
//
//  for (const auto& child : ast->getChildren()) {
//    if (child->getType() == ASTNode::T_GROUP_OVER_TIMEWINDOW) {
//      return true;
//    }
//  }
//
//  return false;
//}
//
//
bool QueryPlanBuilder::hasJoin(ASTNode* ast) const {
  if (!(*ast == ASTNode::T_SELECT) || ast->getChildren().size() < 2) {
    return false;
  }

  auto from_list = ast->getChildren()[1];
  if (from_list->getType() != ASTNode::T_FROM ||
      from_list->getChildren().size() < 1) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  if (from_list->getChildren().size() > 1) {
    return true;
  }

  //for (const auto& child : ast->getChildren()) {
    //if (child->getType() == ASTNode::T_JOIN) {
    //  return true;
    //}
  //}

  return false;
}

bool QueryPlanBuilder::hasOrderByClause(ASTNode* ast) const {
  if (!(*ast == ASTNode::T_SELECT || *ast == ASTNode::T_SELECT_DEEP) ||
      ast->getChildren().size() < 2) {
    return false;
  }

  for (const auto& child : ast->getChildren()) {
    if (child->getType() == ASTNode::T_ORDER_BY) {
      return true;
    }
  }

  return false;
}

bool QueryPlanBuilder::hasAggregationInSelectList(ASTNode* ast) const {
  if (!(*ast == ASTNode::T_SELECT || *ast == ASTNode::T_SELECT_DEEP) ||
      ast->getChildren().size() < 2) {
    return false;
  }

  auto select_list = ast->getChildren()[0];
  if (!(select_list->getType() == ASTNode::T_SELECT_LIST)) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  return hasAggregationExpression(select_list);
}

bool QueryPlanBuilder::hasAggregationExpression(ASTNode* ast) const {
  if (ast->getType() == ASTNode::T_METHOD_CALL) {
    if (!(ast->getToken() != nullptr)) {
      RAISE(kRuntimeError, "corrupt AST");
    }

    if (symbol_table_->isAggregateFunction(ast->getToken()->getString())) {
      return true;
    }
  }

  for (const auto& child : ast->getChildren()) {
    if (hasAggregationExpression(child)) {
      return true;
    }
  }

  return false;
}

bool QueryPlanBuilder::hasAggregationWithinRecord(ASTNode* ast) const {
  if (ast->getType() == ASTNode::T_METHOD_CALL_WITHIN_RECORD) {
    return true;
  }

  for (const auto& child : ast->getChildren()) {
    if (hasAggregationWithinRecord(child)) {
      return true;
    }
  }

  return false;
}

void QueryPlanBuilder::assignExplicitColumnNames(
    Transaction* txn,
    ASTNode* ast,
    RefPtr<TableProvider> tables) {
  if (ast->getChildren().size() < 1) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  auto select_list = ast->getChildren()[0];
  if (select_list->getType() != ASTNode::T_SELECT_LIST) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  for (const auto& col : select_list->getChildren()) {
    if (col->getType() == ASTNode::T_DERIVED_COLUMN &&
        col->getChildren().size() == 1) {
      auto alias = col->appendChild(ASTNode::T_COLUMN_ALIAS);
      alias->setToken(
          new Token(
              Token::T_IDENTIFIER,
              ASTUtil::columnNameForExpression(col->getChildren()[0])));
    }
  }
}

QueryTreeNode* QueryPlanBuilder::buildGroupBy(
    Transaction* txn,
    ASTNode* ast,
    RefPtr<TableProvider> tables) {
  /* copy own select list */
  if (!(ast->getChildren()[0]->getType() == ASTNode::T_SELECT_LIST)) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  auto select_list = ast->getChildren()[0]->deepCopy();

  /* copy ast for child and swap out select lists*/
  auto child_sl = new ASTNode(ASTNode::T_SELECT_LIST);
  auto child_ast = ast->deepCopy();
  child_ast->removeChildrenByType(ASTNode::T_GROUP_BY);
  child_ast->removeChildByIndex(0);
  child_ast->appendChild(child_sl, 0);

  auto subtree = build(txn, child_ast, tables);
  auto subtree_tbl = subtree.asInstanceOf<TableExpressionNode>();

  /* search for a group by clause */
  Vector<RefPtr<ValueExpressionNode>> group_expressions;
  for (const auto& child : ast->getChildren()) {
    if (child->getType() != ASTNode::T_GROUP_BY) {
      continue;
    }

    for (const auto& group_expr : child->getChildren()) {
      auto e = group_expr->deepCopy();
      if (hasAggregationExpression(e)) {
        RAISE(kRuntimeError, "GROUP clause can only contain pure functions");
      }

      group_expressions.emplace_back(
          buildValueExpression(
              txn,
              e,
              std::bind(
                  &TableExpressionNode::getComputedColumnInfo,
                  subtree_tbl.get(),
                  std::placeholders::_1,
                  true),
              std::bind(
                  &resolveColumnType,
                  subtree_tbl.get(),
                  std::placeholders::_1)));
    }
  }

  /* select list  */
  Vector<RefPtr<SelectListNode>> select_list_expressions;
  for (const auto& select_expr : select_list->getChildren()) {
    if (*select_expr == ASTNode::T_ALL) {
      for (const auto& col : subtree_tbl->getAvailableColumns()) {
        auto colnode = new ColumnReferenceNode(col.qualified_name, col.type);
        colnode->setColumnIndex(subtree_tbl->getComputedColumnIndex(col.qualified_name, true));
        auto sl = new SelectListNode(colnode);
        sl->setAlias(col.short_name);
        select_list_expressions.emplace_back(sl);
      }
    } else {
      auto sl = buildSelectList(
          txn,
          select_expr,
          std::bind(
              &TableExpressionNode::getComputedColumnInfo,
              subtree_tbl.get(),
              std::placeholders::_1,
              false),
          std::bind(
              &resolveColumnType,
              subtree_tbl.get(),
              std::placeholders::_1));

      select_list_expressions.emplace_back(sl);
    }
  }

  return new GroupByNode(
      select_list_expressions,
      group_expressions,
      subtree);
}

QueryTreeNode* QueryPlanBuilder::buildLimitClause(
    Transaction* txn,
    ASTNode* ast,
    RefPtr<TableProvider> tables) {
  if (!(*ast == ASTNode::T_SELECT || *ast == ASTNode::T_SELECT_DEEP) ||
      ast->getChildren().size() < 3) {
    return nullptr;
  }

  for (const auto& child : ast->getChildren()) {
    int limit = 0;
    int offset = 0;

    if (child->getType() != ASTNode::T_LIMIT) {
      continue;
    }

    auto limit_token = child->getToken();
    if (!(limit_token)) {
      RAISE(kRuntimeError, "corrupt AST");
    }

    if (!(*limit_token == Token::T_NUMERIC)) {
      RAISE(kRuntimeError, "corrupt AST");
    }

    limit = limit_token->getInteger();

    if (child->getChildren().size() == 1) {
      if (!(child->getChildren()[0]->getType() == ASTNode::T_OFFSET)) {
        RAISE(kRuntimeError, "corrupt AST");
      }

      auto offset_token = child->getChildren()[0]->getToken();
      if (!(offset_token)) {
        RAISE(kRuntimeError, "corrupt AST");
      }

      if (!(*offset_token == Token::T_NUMERIC)) {
        RAISE(kRuntimeError, "corrupt AST");
      }
      offset = offset_token->getInteger();
    }

    // clone ast + remove limit clause
    auto new_ast = ast->deepCopy();
    new_ast->removeChildrenByType(ASTNode::T_LIMIT);

    return new LimitNode(
        limit,
        offset,
        build(txn, new_ast, tables));
  }

  return nullptr;
}

QueryTreeNode* QueryPlanBuilder::buildOrderByClause(
    Transaction* txn,
    ASTNode* ast,
    RefPtr<TableProvider> tables) {
  Vector<OrderByNode::SortSpec> sort_specs;

  /* copy ast for child and remove order by clause */
  auto child_ast = ast->deepCopy();
  child_ast->removeChildrenByType(ASTNode::T_ORDER_BY);
  auto subtree = build(txn, child_ast, tables);
  auto subtree_tbl = subtree.asInstanceOf<TableExpressionNode>();

  /* search for the order by clause */
  for (const auto& child : ast->getChildren()) {
    if (child->getType() != ASTNode::T_ORDER_BY) {
      continue;
    }

    /* build each sort spec and expand select list for missing columns */
    auto sort_specs_asts = child->getChildren();
    for (int i = 0; i < sort_specs_asts.size(); ++i) {
      auto sort = sort_specs_asts[i];

      auto sort_descending = sort->getToken() != nullptr &&
          sort->getToken()->getType() == Token::T_DESC;

      /* create the sort spec */
      OrderByNode::SortSpec sort_spec;
      sort_spec.expr = buildValueExpression(
          txn,
          sort->getChildren()[0],
          std::bind(
              &TableExpressionNode::getComputedColumnInfo,
              subtree_tbl.get(),
              std::placeholders::_1,
              true),
          std::bind(
              &resolveColumnType,
              subtree_tbl.get(),
              std::placeholders::_1));

      sort_spec.descending = sort_descending;
      sort_specs.emplace_back(sort_spec);
    }
  }

  return new OrderByNode(
      sort_specs,
      subtree);
}

QueryTreeNode* QueryPlanBuilder::buildSequentialScan(
    Transaction* txn,
    ASTNode* ast,
    RefPtr<TableProvider> tables) {
  if (!(*ast == ASTNode::T_SELECT || *ast == ASTNode::T_SELECT_DEEP)) {
    return nullptr;
  }

  if (ast->getChildren().size() < 2) {
    return nullptr;
  }

  /* get FROM clause */
  ASTNode* from_list = ast->getChildren()[1];
  if (!(from_list)) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  if (!(*from_list == ASTNode::T_FROM)) {
    return nullptr;
  }

  if (from_list->getChildren().size() < 1) {
    return nullptr;
  }

  /* get table reference */
  auto tbl_name = from_list->getChildren()[0];
  if (!(*tbl_name == ASTNode::T_TABLE_NAME)) {
    return nullptr;
  }

  /* get select list */
  if (!(*ast->getChildren()[0] == ASTNode::T_SELECT_LIST)) {
    RAISE(kRuntimeError, "corrupt AST");
  }
  auto select_list = ast->getChildren()[0];

  /* get where clause */
  ASTNode* where_clause = nullptr;
  if (ast->getChildren().size() > 2) {
    where_clause = ast->getChildren()[2];
  }

  return buildSeqscanTableReference(
      txn,
      from_list,
      select_list,
      where_clause,
      tables,
      false);
}

QueryTreeNode* QueryPlanBuilder::buildSubquery(
    Transaction* txn,
    ASTNode* ast,
    RefPtr<TableProvider> tables) {
  if (!(*ast == ASTNode::T_SELECT || *ast == ASTNode::T_SELECT_DEEP)) {
    return nullptr;
  }

  if (ast->getChildren().size() < 2) {
    return nullptr;
  }

  /* get FROM clause */
  ASTNode* from_list = ast->getChildren()[1];
  if (!(from_list)) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  if (!(*from_list == ASTNode::T_FROM)) {
    return nullptr;
  }

  if (from_list->getChildren().size() < 1) {
    return nullptr;
  }

  if (!(*from_list->getChildren()[0] == ASTNode::T_SELECT)) {
    return nullptr;
  }

  /* get select list */
  if (!(*ast->getChildren()[0] == ASTNode::T_SELECT_LIST)) {
    RAISE(kRuntimeError, "corrupt AST");
  }
  auto select_list = ast->getChildren()[0];

  ASTNode* where_clause = nullptr;
  if (ast->getChildren().size() > 2) {
    where_clause = ast->getChildren()[2];
  }

  return buildSubqueryTableReference(
      txn,
      from_list,
      select_list,
      where_clause,
      tables,
      false);
}

QueryTreeNode* QueryPlanBuilder::buildSelectExpression(
    Transaction* txn,
    ASTNode* ast) {
  if (!(*ast == ASTNode::T_SELECT || *ast == ASTNode::T_SELECT_DEEP)
      || ast->getChildren().size() != 1) {
    return nullptr;
  }

  auto select_list = ast->getChildren()[0];

  /* select list  */
  Vector<RefPtr<SelectListNode>> select_list_expressions;
  for (const auto& select_expr : select_list->getChildren()) {
    if (*select_expr == ASTNode::T_ALL) {
      RAISE(
          kRuntimeError,
          "Illegal use of wildcard * in free SELECT expression");
    }

    if (hasAggregationExpression(select_expr) ||
        hasAggregationWithinRecord(select_expr)) {
      RAISE(
          kRuntimeError,
          "a SELECT without any tables can only contain pure functions");
    }

    select_list_expressions.emplace_back(
        buildSelectList(
            txn,
            select_expr,
            kEmptyColumnResolver,
            kEmptyColumnTypeResolver));
  }

  return new SelectExpressionNode(select_list_expressions);
}

QueryTreeNode* QueryPlanBuilder::buildJoin(
    Transaction* txn,
    ASTNode* ast,
    RefPtr<TableProvider> tables) {
  if (!(*ast == ASTNode::T_SELECT || *ast == ASTNode::T_SELECT_DEEP)) {
    return nullptr;
  }

  if (ast->getChildren().size() < 2) {
    return nullptr;
  }

  /* get FROM clause */
  ASTNode* join = ast->getChildren()[1];
  if (!(join)) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  switch (join->getType()) {
    case ASTNode::T_INNER_JOIN:
    case ASTNode::T_LEFT_JOIN:
    case ASTNode::T_RIGHT_JOIN:
    case ASTNode::T_NATURAL_INNER_JOIN:
    case ASTNode::T_NATURAL_LEFT_JOIN:
    case ASTNode::T_NATURAL_RIGHT_JOIN:
      break;
    default:
      return nullptr;
  }

  /* select list  */
  auto select_list = ast->getChildren()[0];
  ASTNode* where_clause = nullptr;
  if (ast->getChildren().size() > 2) {
    where_clause = ast->getChildren()[2];
  }

  return buildJoinTableReference(
      txn,
      join,
      select_list,
      where_clause,
      tables,
      false);
}

QueryTreeNode* QueryPlanBuilder::buildTableReference(
    Transaction* txn,
    ASTNode* table_ref,
    ASTNode* select_list,
    ASTNode* where_clause,
    RefPtr<TableProvider> tables,
    bool in_join) {
  switch (table_ref->getType()) {
    case ASTNode::T_INNER_JOIN:
    case ASTNode::T_LEFT_JOIN:
    case ASTNode::T_RIGHT_JOIN:
    case ASTNode::T_NATURAL_INNER_JOIN:
    case ASTNode::T_NATURAL_LEFT_JOIN:
    case ASTNode::T_NATURAL_RIGHT_JOIN:
      return buildJoinTableReference(
          txn,
          table_ref,
          select_list,
          where_clause,
          tables,
          in_join);

    case ASTNode::T_FROM:
      if (table_ref->getChildren().size() > 0) {
        switch (table_ref->getChildren()[0]->getType()) {
          case ASTNode::T_TABLE_NAME:
            return buildSeqscanTableReference(
                txn,
                table_ref,
                select_list,
                where_clause,
                tables,
                in_join);

          case ASTNode::T_SELECT:
            return buildSubqueryTableReference(
                txn,
                table_ref,
                select_list,
                where_clause,
                tables,
                in_join);

          default:
            break;

        }
      }
      /* fallthrough */

    default:
      table_ref->debugPrint();
      RAISE(kRuntimeError, "invalid table reference");

  }
}

QueryTreeNode* QueryPlanBuilder::buildJoinTableReference(
    Transaction* txn,
    ASTNode* table_ref,
    ASTNode* select_list,
    ASTNode* where_clause,
    RefPtr<TableProvider> tables,
    bool in_join) {
  if (table_ref->getChildren().size() < 2) {
    return nullptr;
  }

  JoinType join_type;
  bool natural_join = false;
  bool reverse = false;

  switch (table_ref->getType()) {
    case ASTNode::T_NATURAL_INNER_JOIN:
      natural_join = true;
      /* fallthrough */
    case ASTNode::T_INNER_JOIN:
      join_type = JoinType::INNER;
      break;
    case ASTNode::T_NATURAL_LEFT_JOIN:
      natural_join = true;
      /* fallthrough */
    case ASTNode::T_LEFT_JOIN:
      join_type = JoinType::OUTER;
      break;
    case ASTNode::T_NATURAL_RIGHT_JOIN:
      natural_join = true;
      /* fallthrough */
    case ASTNode::T_RIGHT_JOIN:
      reverse = true;
      join_type = JoinType::OUTER;
      break;
    default:
      RAISE(kRuntimeError, "invalid JOIN type");
  }

  auto empty_sl = mkScoped(new ASTNode(ASTNode::T_SELECT_LIST));

  auto base_table = mkRef(
      buildTableReference(
          txn,
          table_ref->getChildren()[0],
          empty_sl.get(),
          where_clause,
          tables,
          true));

  auto joined_table = mkRef(
      buildTableReference(
          txn,
          table_ref->getChildren()[1],
          empty_sl.get(),
          where_clause,
          tables,
          true));

  auto join_node = mkScoped(new JoinNode(
      join_type,
      reverse ? joined_table : base_table,
      reverse ? base_table : joined_table));

  if (!in_join && where_clause) {
    if (!(*where_clause == ASTNode::T_WHERE)) {
      return nullptr;
    }

    if (where_clause->getChildren().size() != 1) {
      RAISE(kRuntimeError, "corrupt AST");
    }

    auto e = where_clause->getChildren()[0];
    if (e == nullptr) {
      RAISE(kRuntimeError, "corrupt AST");
    }

    if (hasAggregationExpression(e)) {
      RAISE(
          kRuntimeError,
          "where expressions can only contain pure functions\n");
    }

    join_node->setWhereExpression(
        buildValueExpression(
            txn,
            e,
            std::bind(
                &JoinNode::getInputColumnInfo,
                join_node.get(),
                std::placeholders::_1,
                true),
            [&join_node] (size_t i) { return join_node->getInputColumnType(i); }));
  }

  Vector<QualifiedColumn> all_columns;

  if (natural_join) {
    RefPtr<TableExpressionNode> primary_table;
    RefPtr<TableExpressionNode> secondary_table;
    if (reverse) {
      primary_table = joined_table.asInstanceOf<TableExpressionNode>();
      secondary_table = base_table.asInstanceOf<TableExpressionNode>();
    } else {
      primary_table = base_table.asInstanceOf<TableExpressionNode>();
      secondary_table = joined_table.asInstanceOf<TableExpressionNode>();
    }

    HashMap<String, std::vector<std::pair<std::string, SType>>> common_columns;
    {
      Set<String> tmp_column_set;
      for (const auto& col : secondary_table->getAvailableColumns()) {
        tmp_column_set.insert(col.short_name);
      }

      for (const auto& col : primary_table->getAvailableColumns()) {
        if (tmp_column_set.count(col.short_name) > 0) {
          all_columns.emplace_back(col);
          common_columns.emplace(col.short_name, std::vector<std::pair<std::string, SType>>{});
          tmp_column_set.erase(col.short_name);
        }
      }
    }

    for (const auto& col :
            base_table.asInstanceOf<TableExpressionNode>()->getAvailableColumns()) {
      if (common_columns.count(col.short_name) == 0) {
        all_columns.emplace_back(col);
      } else {
        common_columns[col.short_name].emplace_back(col.qualified_name, col.type);
      }
    }

    for (const auto& col :
            joined_table.asInstanceOf<TableExpressionNode>()->getAvailableColumns()) {
      if (common_columns.count(col.short_name) == 0) {
        all_columns.emplace_back(col);
      } else {
        common_columns[col.short_name].emplace_back(col.qualified_name, col.type);
      }
    }

    RefPtr<ValueExpressionNode> pred;
    for (const auto& c : common_columns) {
      for (size_t i1 = 0; i1 < c.second.size(); ++i1) {
        for (size_t i2 = 0; i2 < c.second.size(); ++i2) {
          if (i1 == i2) {
            continue;
          }

          RefPtr<ValueExpressionNode> cpred;
          {
            auto rc = csql::CallExpressionNode::newNode(
                txn,
                "eq",
                Vector<RefPtr<csql::ValueExpressionNode>>{
                  new csql::ColumnReferenceNode(c.second[i1].first, c.second[i1].second),
                  new csql::ColumnReferenceNode(c.second[i2].first, c.second[i2].second)
                },
                &cpred);

            if (!rc.isSuccess()) {
              RAISE(kRuntimeError, rc.getMessage());
            }
          }

          if (pred.get() == nullptr) {
            pred = cpred;
          } else {
            auto rc = csql::CallExpressionNode::newNode(
                txn,
                "logical_and",
                Vector<RefPtr<csql::ValueExpressionNode>>{ pred, cpred },
                &pred);

            if (!rc.isSuccess()) {
              RAISE(kRuntimeError, rc.getMessage());
            }
          }
        }
      }
    }

    if (pred.get() != nullptr) {
      join_node->setJoinCondition(pred);
    }
  } else {
    for (const auto& col :
            base_table.asInstanceOf<TableExpressionNode>()->getAvailableColumns()) {
      all_columns.emplace_back(col);
    }

    for (const auto& col :
            joined_table.asInstanceOf<TableExpressionNode>()->getAvailableColumns()) {
      all_columns.emplace_back(col);
    }

    if (table_ref->getChildren().size() > 2) {
      auto cond_ast = table_ref->getChildren()[2];

      switch (cond_ast->getType()) {
        case ASTNode::T_JOIN_CONDITION: {
          if (cond_ast->getChildren().size() != 1) {
            RAISE(kRuntimeError, "corrupt AST");
          }

          auto e = cond_ast->getChildren()[0];
          if (e == nullptr) {
            RAISE(kRuntimeError, "corrupt AST");
          }

          if (hasAggregationExpression(e)) {
            RAISE(
                kRuntimeError,
                "JOIN conditions can only contain pure functions\n");
          }

          join_node->setJoinCondition(
              buildValueExpression(
                  txn,
                  e,
                  std::bind(
                      &JoinNode::getInputColumnInfo,
                      join_node.get(),
                      std::placeholders::_1,
                      true),
                  [&join_node] (size_t i) { return join_node->getInputColumnType(i); }));

          break;
        }

        case ASTNode::T_JOIN_COLUMNLIST: {
          RAISE(kNotYetImplementedError);
        }

        default:
          RAISE(kRuntimeError, "corrupt AST");
      }
    }
  }

  for (const auto& select_expr : select_list->getChildren()) {
    if (hasAggregationWithinRecord(select_expr)) {
      RAISE(
          kRuntimeError,
          "WITHIN RECORD can't be used together with JOIN in the same SELECT"
          " statement. consider moving the WITHIN RECORD expression into a"
          " subquery");
    }

    if (*select_expr == ASTNode::T_ALL) {
      for (const auto& col : all_columns) {
        auto sl = new SelectListNode(
            new ColumnReferenceNode(col.qualified_name, col.type));
        sl->setAlias(col.short_name);
        join_node->addSelectList(sl);
      }
    } else {
      join_node->addSelectList(
          buildSelectList(
              txn,
              select_expr,
              std::bind(
                  &JoinNode::getInputColumnInfo,
                  join_node.get(),
                  std::placeholders::_1,
                  true),
              [&join_node] (size_t i) { return join_node->getInputColumnType(i); }));
    }
  }

  if (join_node->joinCondition().isEmpty() &&
      join_node->joinType() == JoinType::INNER) {
    join_node->setJoinType(JoinType::CARTESIAN);
  }

  return join_node.release();
}

QueryTreeNode* QueryPlanBuilder::buildSubqueryTableReference(
    Transaction* txn,
    ASTNode* table_ref,
    ASTNode* select_list,
    ASTNode* where_clause,
    RefPtr<TableProvider> tables,
    bool in_join) {
  if (!(*table_ref == ASTNode::T_FROM)) {
    return nullptr;
  }

  if (table_ref->getChildren().size() < 1) {
    return nullptr;
  }

  /* get subquery */
  auto subquery_ast = table_ref->getChildren()[0];
  if (!(*subquery_ast == ASTNode::T_SELECT)) {
    return nullptr;
  }

  String subquery_alias;
  /* .. AS alias */
  if (table_ref->getChildren().size() > 1 &&
      table_ref->getChildren()[1]->getType() == ASTNode::T_TABLE_ALIAS) {
    subquery_alias = table_ref->getChildren()[1]->getToken()->getString();
  }

  auto subquery = build(txn, subquery_ast, tables);
  auto subquery_tbl = subquery.asInstanceOf<TableExpressionNode>();
  auto resolver = [&subquery_tbl, &subquery_alias] (const String& name) {
    auto col = name;
    if (!subquery_alias.empty()) {
      if (StringUtil::beginsWith(col, subquery_alias + ".")) {
        col = col.substr(subquery_alias.size() + 1);
      }
    }

    return subquery_tbl->getComputedColumnInfo(col);
  };

  auto type_resolver = std::bind(
      &resolveColumnType,
      subquery_tbl.get(),
      std::placeholders::_1);

  Vector<RefPtr<SelectListNode>> select_list_expressions;
  for (const auto& select_expr : select_list->getChildren()) {
    if (*select_expr == ASTNode::T_ALL) {
      for (const auto& col : subquery_tbl->getResultColumns()) {
        auto colnode = new ColumnReferenceNode(
            col,
            subquery_tbl->getColumnType(
                subquery_tbl->getComputedColumnIndex(col)));
        colnode->setColumnIndex(subquery_tbl->getComputedColumnIndex(col));

        auto sl = new SelectListNode(colnode);
        sl->setAlias(col);
        select_list_expressions.emplace_back(sl);
      }
    } else {
      auto sl = buildSelectList(txn, select_expr, resolver, type_resolver);
      select_list_expressions.emplace_back(sl);
    }
  }

  /* get where expression */
  Option<RefPtr<ValueExpressionNode>> where_expr;
  if (!in_join && where_clause) {
    if (!(*where_clause == ASTNode::T_WHERE)) {
      return nullptr;
    }

    if (where_clause->getChildren().size() != 1) {
      RAISE(kRuntimeError, "corrupt AST");
    }

    auto e = where_clause->getChildren()[0];

    if (e == nullptr) {
      RAISE(kRuntimeError, "corrupt AST");
    }

    if (hasAggregationExpression(e)) {
      RAISE(
          kRuntimeError,
          "where expressions can only contain pure functions\n");
    }

    where_expr = Some(RefPtr<ValueExpressionNode>(buildValueExpression(txn, e, resolver, type_resolver)));
  }

  auto subqnode = new SubqueryNode(
      subquery,
      select_list_expressions,
      where_expr);
  subqnode->setTableAlias(subquery_alias);

  return subqnode;
}

QueryTreeNode* QueryPlanBuilder::buildSeqscanTableReference(
    Transaction* txn,
    ASTNode* table_ref,
    ASTNode* select_list,
    ASTNode* where_clause,
    RefPtr<TableProvider> tables,
    bool in_join) {
  if (!(*table_ref == ASTNode::T_FROM)) {
    return nullptr;
  }

  if (table_ref->getChildren().size() < 1) {
    return nullptr;
  }

  /* get table reference */
  auto tbl_name = table_ref->getChildren()[0];
  if (!(*tbl_name == ASTNode::T_TABLE_NAME)) {
    return nullptr;
  }

  auto tbl_name_token = tbl_name->getToken();
  if (!(tbl_name_token != nullptr)) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  auto table_name = tbl_name_token->getString();

  /* get table alias */
  String table_alias;
  if (table_ref->getChildren().size() > 1 &&
      table_ref->getChildren()[1]->getType() == ASTNode::T_TABLE_ALIAS) {
    table_alias = table_ref->getChildren()[1]->getToken()->getString();
  }

  auto table = tables->describe(table_name);
  if (table.isEmpty()) {
    RAISEF(kRuntimeError, "table not found: '$0'", table_name);
  }

  /* build ndoe */
  auto seqscan = mkRef(new SequentialScanNode(table.get(), tables));
  if (!table_alias.empty()) {
    seqscan->setTableAlias(table_alias);
  }

  /* get where expression */
  if (where_clause && !in_join) {
    if (!(*where_clause == ASTNode::T_WHERE)) {
      return nullptr;
    }

    if (where_clause->getChildren().size() != 1) {
      RAISE(kRuntimeError, "corrupt AST");
    }

    auto e = where_clause->getChildren()[0];

    if (e == nullptr) {
      RAISE(kRuntimeError, "corrupt AST");
    }

    if (hasAggregationExpression(e)) {
      RAISE(
          kRuntimeError,
          "where expressions can only contain pure functions\n");
    }

    auto pred = RefPtr<ValueExpressionNode>(
        buildValueExpression(
            txn,
            e,
            std::bind(
                &SequentialScanNode::getInputColumnInfo,
                seqscan.get(),
                std::placeholders::_1,
                true),
            [seqscan] (size_t i) { return seqscan->getInputColumnType(i); }));

    // FIXME skip if literal true expression
    seqscan->setWhereExpression(pred);
  }

  bool has_aggregation = false;
  bool has_aggregation_within_record = false;

  /* select list  */
  Vector<RefPtr<SelectListNode>> select_list_expressions;
  for (const auto& select_expr : select_list->getChildren()) {
    if (*select_expr == ASTNode::T_ALL) {
      for (const auto& col : table.get().columns) {
        auto colnode = new ColumnReferenceNode(col.column_name, col.type);
        colnode->setColumnIndex(seqscan->getInputColumnIndex(col.column_name, true));
        auto sl = new SelectListNode(colnode);
        sl->setAlias(col.column_name);
        seqscan->addSelectList(sl);
      }
    } else {
      if (hasAggregationExpression(select_expr)) {
        has_aggregation = true;
      }

      if (hasAggregationWithinRecord(select_expr)) {
        has_aggregation_within_record = true;
      }

      seqscan->addSelectList(
          buildSelectList(
              txn,
              select_expr,
              std::bind(
                  &SequentialScanNode::getInputColumnInfo,
                  seqscan.get(),
                  std::placeholders::_1,
                  true),
              [seqscan] (size_t i) { return seqscan->getInputColumnType(i); }));
    }
  }

  if (has_aggregation && has_aggregation_within_record) {
    RAISE(
        kRuntimeError,
        "invalid use of aggregation WITHIN RECORD functions");
  }

  if (has_aggregation) {
    seqscan->setAggregationStrategy(AggregationStrategy::AGGREGATE_ALL);
  }

  if (has_aggregation_within_record) {
    seqscan->setAggregationStrategy(AggregationStrategy::AGGREGATE_WITHIN_RECORD_FLAT);
  }

  seqscan->normalizeColumnNames();
  return seqscan.release();
}

RefPtr<ValueExpressionNode> QueryPlanBuilder::buildValueExpression(
    Transaction* txn,
    ASTNode* ast,
    ColumnResolver resolver,
    ColumnTypeResolver type_resolver) {
  auto valexpr = buildUnoptimizedValueExpression(txn, ast, resolver, type_resolver);

  if (opts_.enable_constant_folding) {
    valexpr = QueryTreeUtil::foldConstants(txn, valexpr);
  }

  return valexpr;
}

RefPtr<ValueExpressionNode> QueryPlanBuilder::buildUnoptimizedValueExpression(
    Transaction* txn,
    ASTNode* ast,
    ColumnResolver resolver,
    ColumnTypeResolver type_resolver) {
  if (ast == nullptr) {
    RAISE(kNullPointerError, "can't build nullptr");
  }

  switch (ast->getType()) {

    case ASTNode::T_EQ_EXPR:
      return buildOperator(txn, "eq", ast, resolver, type_resolver);

    case ASTNode::T_NEQ_EXPR:
      return buildOperator(txn, "neq", ast, resolver, type_resolver);

    case ASTNode::T_AND_EXPR:
      return buildOperator(txn, "logical_and", ast, resolver, type_resolver);

    case ASTNode::T_OR_EXPR:
      return buildOperator(txn, "logical_or", ast, resolver, type_resolver);

    case ASTNode::T_NEGATE_EXPR:
      return buildOperator(txn, "neg", ast, resolver, type_resolver);

    case ASTNode::T_LT_EXPR:
      return buildOperator(txn, "lt", ast, resolver, type_resolver);

    case ASTNode::T_LTE_EXPR:
      return buildOperator(txn, "lte", ast, resolver, type_resolver);

    case ASTNode::T_GT_EXPR:
      return buildOperator(txn, "gt", ast, resolver, type_resolver);

    case ASTNode::T_GTE_EXPR:
      return buildOperator(txn, "gte", ast, resolver, type_resolver);

    case ASTNode::T_ADD_EXPR:
      return buildOperator(txn, "add", ast, resolver, type_resolver);

    case ASTNode::T_SUB_EXPR:
      return buildOperator(txn, "sub", ast, resolver, type_resolver);

    case ASTNode::T_MUL_EXPR:
      return buildOperator(txn, "mul", ast, resolver, type_resolver);

    case ASTNode::T_DIV_EXPR:
      return buildOperator(txn, "div", ast, resolver, type_resolver);

    case ASTNode::T_MOD_EXPR:
      return buildOperator(txn, "mod", ast, resolver, type_resolver);

    case ASTNode::T_POW_EXPR:
      return buildOperator(txn, "pow", ast, resolver, type_resolver);

    case ASTNode::T_REGEX_EXPR:
      return buildRegex(txn, ast, resolver, type_resolver);

    case ASTNode::T_LIKE_EXPR:
      return buildLike(txn, ast, resolver, type_resolver);

    case ASTNode::T_LITERAL:
      return buildLiteral(txn, ast);

    case ASTNode::T_VOID:
      return new LiteralExpressionNode(SValue::newNull());

    case ASTNode::T_IF_EXPR:
      return buildIfStatement(txn, ast, resolver, type_resolver);

    case ASTNode::T_COLUMN_NAME:
      return buildColumnReference(txn, ast, resolver);

    case ASTNode::T_COLUMN_INDEX:
    case ASTNode::T_RESOLVED_COLUMN:
      return buildColumnIndex(txn, ast, type_resolver);

    case ASTNode::T_TABLE_NAME:
      return buildColumnReference(txn, ast->getChildren()[0], resolver);

    case ASTNode::T_METHOD_CALL:
    case ASTNode::T_METHOD_CALL_WITHIN_RECORD:
      return buildMethodCall(txn, ast, resolver, type_resolver);

    default:
      ast->debugPrint();
      RAISE(kRuntimeError, "internal error: can't build expression");
  }
}

ValueExpressionNode* QueryPlanBuilder::buildLiteral(
    Transaction* txn,
    ASTNode* ast) {
  if (ast->getToken() == nullptr) {
    RAISE(kRuntimeError, "internal error: corrupt ast");
  }

  SValue literal;
  auto token = ast->getToken();

  switch (token->getType()) {

    case Token::T_TRUE:
      literal = SValue::newBool(true);
      break;

    case Token::T_FALSE:
      literal = SValue::newBool(false);
      break;

    case Token::T_NUMERIC:
      if (token->getString().find(".") == std::string::npos) {
        if (token->getString().find("-") == std::string::npos) {
          literal = SValue::newUInt64(token->getString());
        } else {
          literal = SValue::newInt64(token->getString());
        }
      } else {
        literal = SValue::newFloat64(token->getString());
      }
      break;

    case Token::T_STRING:
      literal = SValue::newString(token->getString());
      break;

    case Token::T_NULL:
      literal = SValue{};
      break;

    default:
      RAISE(kRuntimeError, "can't cast Token to SValue");

  }

  return new LiteralExpressionNode(literal);
}

ValueExpressionNode* QueryPlanBuilder::buildOperator(
    Transaction* txn,
    const std::string& name,
    ASTNode* ast,
    ColumnResolver resolver,
    ColumnTypeResolver type_resolver) {
  Vector<RefPtr<ValueExpressionNode>> args;
  for (auto e : ast->getChildren()) {
    args.emplace_back(buildValueExpression(txn, e, resolver, type_resolver));
  }

  RefPtr<ValueExpressionNode> node;
  auto rc = CallExpressionNode::newNode(txn, name, args, &node);

  if (!rc.isSuccess()) {
    RAISE(kRuntimeError, rc.getMessage());
  }

  return node.release();
}

ValueExpressionNode* QueryPlanBuilder::buildMethodCall(
    Transaction* txn,
    ASTNode* ast,
    ColumnResolver resolver,
    ColumnTypeResolver type_resolver) {
  if (ast->getToken() == nullptr ||
      ast->getToken()->getType() != Token::T_IDENTIFIER) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  auto symbol = ast->getToken()->getString();

  Vector<RefPtr<ValueExpressionNode>> args;
  for (auto e : ast->getChildren()) {
    args.emplace_back(buildValueExpression(txn, e, resolver, type_resolver));
  }

  RefPtr<ValueExpressionNode> node;
  auto rc = CallExpressionNode::newNode(txn, symbol, args, &node);

  if (!rc.isSuccess()) {
    RAISE(kRuntimeError, rc.getMessage());
  }

  return node.release();
}

ValueExpressionNode* QueryPlanBuilder::buildIfStatement(
    Transaction* txn,
    ASTNode* ast,
    ColumnResolver resolver,
    ColumnTypeResolver type_resolver) {
  Vector<RefPtr<ValueExpressionNode>> args;
  for (auto e : ast->getChildren()) {
    args.emplace_back(buildValueExpression(txn, e, resolver, type_resolver));
  }

  if (args.size() != 3) {
    RAISE(kRuntimeError, "if statement must have exactly 3 arguments");
  }

  RefPtr<ValueExpressionNode> node;
  auto rc = IfExpressionNode::newNode(args[0], args[1], args[2], &node);

  if (!rc.isSuccess()) {
    RAISE(kRuntimeError, rc.getMessage());
  }

  return node.release();
}

ValueExpressionNode* QueryPlanBuilder::buildColumnReference(
    Transaction* txn,
    ASTNode* ast,
    ColumnResolver resolver) {
  String column_name;

  for (
      auto cur = ast;
      cur->getToken() != nullptr &&
      cur->getToken()->getType() == Token::T_IDENTIFIER; ) {
    if (cur != ast) column_name += ".";
    column_name += cur->getToken()->getString();

    if (cur->getChildren().size() != 1) {
      break;
    } else {
      cur = cur->getChildren()[0];
    }
  }

  assert(!column_name.empty());

  auto colinfo = resolver(column_name);
  if (colinfo.first == size_t(-1)) {
    RAISEF(kRuntimeError, "column(s) not found: '$0'", column_name);
  }

  auto colref = new ColumnReferenceNode(column_name, colinfo.second);
  colref->setColumnIndex(colinfo.first);
  return colref;
}

ValueExpressionNode* QueryPlanBuilder::buildColumnIndex(
    Transaction* txn,
    ASTNode* ast,
    ColumnTypeResolver type_resolver) {
  if (*ast == ASTNode::T_RESOLVED_COLUMN) {
    return new ColumnReferenceNode(
        ast->getID(),
        type_resolver(ast->getID()));
  }

  if (ast->getChildren().size() != 1) {
    RAISE(kRuntimeError, "internal error: invalid column index reference");
  }

  auto token = ast->getChildren()[0]->getToken();
  if (!token || token->getType() != Token::T_NUMERIC) {
    RAISE(kRuntimeError, "internal error: invalid column index reference");
  }

  return new ColumnReferenceNode(
      token->getInteger(),
      type_resolver(token->getInteger()));
}

ValueExpressionNode* QueryPlanBuilder::buildRegex(
    Transaction* txn,
    ASTNode* ast,
    ColumnResolver resolver,
    ColumnTypeResolver type_resolver) {
  const auto& args = ast->getChildren();
  if (args.size() != 2) {
    RAISE(kRuntimeError, "internal error: corrupt ast");
  }

  if (args[1]->getType() != ASTNode::T_LITERAL ||
      args[1]->getToken() == nullptr ||
      args[1]->getToken()->getType() != Token::T_STRING) {
    RAISE(
        kRuntimeError,
        "second argument to REGEX operator must be a string literal");
  }

  auto pattern = args[1]->getToken()->getString();
  auto subject = buildValueExpression(txn, args[0], resolver, type_resolver);

  return new RegexExpressionNode(subject, pattern);
}

ValueExpressionNode* QueryPlanBuilder::buildLike(
    Transaction* txn,
    ASTNode* ast,
    ColumnResolver resolver,
    ColumnTypeResolver type_resolver) {
  const auto& args = ast->getChildren();
  if (args.size() != 2) {
    RAISE(kRuntimeError, "internal error: corrupt ast");
  }

  if (args[1]->getType() != ASTNode::T_LITERAL ||
      args[1]->getToken() == nullptr ||
      args[1]->getToken()->getType() != Token::T_STRING) {
    RAISE(
        kRuntimeError,
        "second argument to LIKE operator must be a string literal");
  }

  auto pattern = args[1]->getToken()->getString();
  auto subject = buildValueExpression(txn, args[0], resolver, type_resolver);

  return new LikeExpressionNode(subject, pattern);
}

SelectListNode* QueryPlanBuilder::buildSelectList(
    Transaction* txn,
    ASTNode* ast,
    ColumnResolver resolver,
    ColumnTypeResolver type_resolver) {
  if (ast->getChildren().size() == 0) {
    RAISE(kRuntimeError, "internal error: corrupt ast");
  }

  auto slnode = new SelectListNode(
      buildValueExpression(txn, ast->getChildren()[0], resolver, type_resolver));

  /* .. AS alias */
  if (ast->getType() == ASTNode::T_DERIVED_COLUMN &&
      ast->getChildren().size() > 1 &&
      ast->getChildren()[1]->getType() == ASTNode::T_COLUMN_ALIAS) {
    slnode->setAlias(ast->getChildren()[1]->getToken()->getString());
  };

  return slnode;
}

QueryTreeNode* QueryPlanBuilder::buildShowTables(
    Transaction* txn,
    ASTNode* ast) {
  if (!(*ast == ASTNode::T_SHOW_TABLES)) {
    return nullptr;
  }

  return new ShowTablesNode();
}

QueryTreeNode* QueryPlanBuilder::buildDescribeTable(
    Transaction* txn,
    ASTNode* ast) {
  if (!(*ast == ASTNode::T_DESCRIBE_TABLE) || ast->getChildren().size() != 1) {
    return nullptr;
  }

  auto table_name = ast->getChildren()[0];
  if (table_name->getType() != ASTNode::T_TABLE_NAME ||
      table_name->getToken() == nullptr) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  return new DescribeTableNode(table_name->getToken()->getString());
}

QueryTreeNode* QueryPlanBuilder::buildDescribePartitions(
    Transaction* txn,
    ASTNode* ast) {
  if (!(*ast == ASTNode::T_DESCRIBE_PARTITIONS) ||
        ast->getChildren().size() != 1) {
    return nullptr;
  }

  auto table_name = ast->getChildren()[0];
  if (table_name->getType() != ASTNode::T_TABLE_NAME ||
      table_name->getToken() == nullptr) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  return new DescribePartitionsNode(table_name->getToken()->getString());
}

QueryTreeNode* QueryPlanBuilder::buildClusterShowServers(
    Transaction* txn,
    ASTNode* ast) {
  if (!(*ast == ASTNode::T_CLUSTER_SHOW_SERVERS) ||
        ast->getChildren().size() != 0) {
    return nullptr;
  }

  return new ClusterShowServersNode();
};

static TableSchema buildCreateTableSchema(ASTNode* ast);

static void buildCreateTableSchemaColumn(
    ASTNode* ast,
    TableSchemaBuilder* schema) {
  if (ast->getChildren().size() < 2) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  auto column_name = ast->getChildren()[0];
  if (column_name->getType() != ASTNode::T_COLUMN_NAME ||
      column_name->getToken() == nullptr) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  Vector<TableSchema::ColumnOptions> column_options;
  for (size_t i = 2; i < ast->getChildren().size(); ++i) {
    switch (ast->getChildren()[i]->getType()) {
      case ASTNode::T_NOT_NULL:
        column_options.emplace_back(TableSchema::ColumnOptions::NOT_NULL);
        break;
      case ASTNode::T_REPEATED:
        column_options.emplace_back(TableSchema::ColumnOptions::REPEATED);
        break;
      case ASTNode::T_PRIMARY_KEY:
        column_options.emplace_back(TableSchema::ColumnOptions::PRIMARY_KEY);
        break;
      default:
        RAISE(kRuntimeError, "corrupt AST");
    }
  }

  switch (ast->getChildren()[1]->getType()) {

    case ASTNode::T_COLUMN_TYPE: {
      auto column_type = ast->getChildren()[1];
      if (column_type->getType() != ASTNode::T_COLUMN_TYPE ||
          column_type->getToken() == nullptr) {
        RAISE(kRuntimeError, "corrupt AST");
      }

      schema->addScalarColumn(
          column_name->getToken()->getString(),
          column_type->getToken()->getString(),
          column_options);

      break;
    }

    case ASTNode::T_RECORD: {
      schema->addRecordColumn(
          column_name->getToken()->getString(),
          column_options,
          buildCreateTableSchema(ast->getChildren()[1]));

      break;
    }

    default:
      RAISE(kRuntimeError, "corrupt AST");
  }
}

static TableSchema buildCreateTableSchema(ASTNode* ast) {
  TableSchemaBuilder schema_builder;

  switch (ast->getType()) {
    case ASTNode::T_COLUMN_LIST:
    case ASTNode::T_RECORD:
      break;
    default:
      RAISE(kRuntimeError, "corrupt AST");
  }

  for (const auto& cld : ast->getChildren()) {
    switch (cld->getType()) {

      case ASTNode::T_COLUMN: {
        buildCreateTableSchemaColumn(cld, &schema_builder);
        break;
      }

      case ASTNode::T_PRIMARY_KEY: {
        if (ast->getType() == ASTNode::T_RECORD) {
          RAISE(
              kRuntimeError,
              "invalid column definition: can't use PRIMARY_KEY() within RECORD");
        }
        break;
      }

      case ASTNode::T_PARTITION_KEY:
        break;

      default:
        RAISE(kRuntimeError, "corrupt AST");

    }
  }

  return schema_builder.getTableSchema();
}

QueryTreeNode* QueryPlanBuilder::buildCreateTable(
    Transaction* txn,
    ASTNode* ast) {
  if (!(*ast == ASTNode::T_CREATE_TABLE) || ast->getChildren().size() < 2) {
    return nullptr;
  }

  auto table_name = ast->getChildren()[0];
  if (table_name->getType() != ASTNode::T_TABLE_NAME ||
      table_name->getToken() == nullptr) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  auto table_schema = buildCreateTableSchema(ast->getChildren()[1]);
  Vector<String> primary_key_columns;
  std::string partition_key;

  for (const auto& cld : ast->getChildren()[1]->getChildren()) {
    if (cld->getType() != ASTNode::T_PRIMARY_KEY) {
      continue;
    }

    if (!primary_key_columns.empty()) {
      RAISE(kRuntimeError, "can't have more than one PRIMARY KEY definition");
    }

    for (const auto& col : cld->getChildren()) {
      if (col->getType() != ASTNode::T_COLUMN_NAME ||
          col->getToken() == nullptr) {
        RAISE(kRuntimeError, "corrupt AST");
      }

      primary_key_columns.emplace_back(col->getToken()->getString());
    }
  }

  for (const auto& cld : ast->getChildren()[1]->getChildren()) {
    if (cld->getType() != ASTNode::T_PARTITION_KEY) {
      continue;
    }

    if (!partition_key.empty()) {
      RAISE(kRuntimeError, "can't have more than one PARTITION KEY definition");
    }

    for (const auto& col : cld->getChildren()) {
      if (col->getType() != ASTNode::T_COLUMN_NAME ||
          col->getToken() == nullptr) {
        RAISE(kRuntimeError, "corrupt AST");
      }

      partition_key = col->getToken()->getString();
    }
  }

  for (auto col : table_schema.getFlatColumnList()) {
    bool is_primary_key = std::find(
        col->column_options.begin(),
        col->column_options.end(),
        TableSchema::ColumnOptions::PRIMARY_KEY) != col->column_options.end();

    if (!is_primary_key) {
      continue;
    }

    if (!primary_key_columns.empty()) {
      RAISE(kRuntimeError, "can't have more than one PRIMARY KEY definition");
    }

    primary_key_columns.emplace_back(col->full_column_name);
  }

  if (primary_key_columns.empty()) {
    RAISE(kRuntimeError, "table must have a PRIMARY KEY");
  }

  if (!partition_key.empty() && partition_key != primary_key_columns[0]) {
    RAISE(
        kRuntimeError,
        "PARTITION KEY must match the PRIMARY KEY (or the first column of the PRIMARY KEY for compound PRIMARY KEYs)");
  }

  auto node = new CreateTableNode(
      table_name->getToken()->getString(),
      std::move(table_schema));

  if (!primary_key_columns.empty()) {
    node->setPrimaryKey(primary_key_columns);
  }

  if (!partition_key.empty()) {
    node->setPartitionKey(partition_key);
  }

  if (ast->getChildren().size() >= 3) {
    for (const auto& cld : ast->getChildren()[2]->getChildren()) {
      if (cld->getType() != ASTNode::T_TABLE_PROPERTY) {
        continue;
      }

      if (cld->getChildren().size() != 2 ||
          cld->getChildren()[0]->getType() != ASTNode::T_TABLE_PROPERTY_KEY ||
          cld->getChildren()[1]->getType() != ASTNode::T_TABLE_PROPERTY_VALUE) {
        RAISE(kRuntimeError, "corrupt AST");
      }

      std::string value;
      switch (cld->getChildren()[1]->getToken()->getType()) {
        case Token::T_TRUE:
          value = "true";
          break;
        case Token::T_FALSE:
          value = "false";
          break;
        default:
          value = cld->getChildren()[1]->getToken()->getString();
          break;
      }

      node->addProperty(
          cld->getChildren()[0]->getToken()->getString(),
          value);
    }
  }

  return node;
}

QueryTreeNode* QueryPlanBuilder::buildCreateDatabase(
    Transaction* txn,
    ASTNode* ast) {
  if (!(*ast == ASTNode::T_CREATE_DATABASE) || ast->getChildren().size() != 1) {
    return nullptr;
  }

  auto db_name = ast->getChildren()[0];
  if (db_name->getType() != ASTNode::T_DATABASE_NAME ||
      db_name->getToken() == nullptr) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  return new CreateDatabaseNode(db_name->getToken()->getString());
}

QueryTreeNode* QueryPlanBuilder::buildUseDatabase(
    Transaction* txn,
    ASTNode* ast) {
  if (!(*ast == ASTNode::T_USE_DATABASE) || ast->getChildren().size() != 1) {
    return nullptr;
  }

  auto db_name = ast->getChildren()[0];
  if (db_name->getType() != ASTNode::T_DATABASE_NAME ||
      db_name->getToken() == nullptr) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  return new UseDatabaseNode(db_name->getToken()->getString());
}

QueryTreeNode* QueryPlanBuilder::buildDropTable(
    Transaction* txn,
    ASTNode* ast) {
  if (!(*ast == ASTNode::T_DROP_TABLE) || ast->getChildren().size() != 1) {
    return nullptr;
  }

  auto db_name = ast->getChildren()[0];
  if (db_name->getType() != ASTNode::T_TABLE_NAME ||
      db_name->getToken() == nullptr) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  return new DropTableNode(db_name->getToken()->getString());
}

QueryTreeNode* QueryPlanBuilder::buildInsertInto(
    Transaction* txn,
    ASTNode* ast) {
  if (!(*ast == ASTNode::T_INSERT_INTO) || ast->getChildren().size() < 2) {
    return nullptr;
  }

  auto table_name = ast->getChildren()[0];
  if (table_name->getType() != ASTNode::T_TABLE_NAME ||
      table_name->getToken() == nullptr) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  if (ast->getChildren()[1]->getType() == ASTNode::T_JSON_STRING &&
      ast->getChildren()[1]->getToken() != nullptr) {
    return new InsertJSONNode(
        table_name->getToken()->getString(),
        ast->getChildren()[1]->getToken()->getString());
  }

  if (ast->getChildren()[1]->getType() != ASTNode::T_COLUMN_LIST ||
      ast->getChildren().size() < 3 ||
      ast->getChildren()[2]->getType() != ASTNode::T_VALUE_LIST) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  auto columns = ast->getChildren()[1]->getChildren();
  auto values = ast->getChildren()[2]->getChildren();

  Vector<InsertIntoNode::InsertValueSpec> values_spec;
  for (size_t i = 0; i < values.size(); ++i) {
    InsertIntoNode::InsertValueSpec spec;
    spec.type = InsertIntoNode::InsertValueType::SCALAR;
    spec.expr = buildValueExpression(
        txn,
        values[i],
        kEmptyColumnResolver,
        kEmptyColumnTypeResolver);

    if (columns.size() > i &&
        columns[i]->getType() == ASTNode::T_COLUMN_NAME &&
        columns[i]->getToken() != nullptr) {
      spec.column = columns[i]->getToken()->getString();
    }

    values_spec.emplace_back(spec);
  }

  return new InsertIntoNode(table_name->getToken()->getString(), values_spec);
}

static AlterTableNode::AlterTableOperation buildAlterTableOperation(
    ASTNode* ast) {

  switch (ast->getType()) {
    //drop column
    case ASTNode::T_COLUMN_NAME:
      if (ast->getToken() != nullptr) {
        AlterTableNode::AlterTableOperation operation;
        operation.optype =
            AlterTableNode::AlterTableOperationType::OP_REMOVE_COLUMN;
        operation.column_name = ast->getToken()->getString();
        return operation;
      }

    //add column
    case ASTNode::T_COLUMN: {
        if (ast->getChildren().size() < 2) {
          RAISE(kRuntimeError, "corrupt AST");
        }

        if (!(*ast->getChildren()[0] == ASTNode::T_COLUMN_NAME) ||
            ast->getChildren()[0]->getToken() == nullptr) {
          RAISE(kRuntimeError, "corrupt AST");
        }

        AlterTableNode::AlterTableOperation operation;
        operation.optype =
            AlterTableNode::AlterTableOperationType::OP_ADD_COLUMN;
        operation.column_name = ast->getChildren()[0]->getToken()->getString();
        operation.is_repeated = false;
        operation.is_optional = true;

        switch (ast->getChildren()[1]->getType()) {
          case ASTNode::T_RECORD:
            operation.column_type = "RECORD";
            break;

          case ASTNode::T_COLUMN_TYPE:
            if (ast->getChildren()[1]->getToken() == nullptr) {
              RAISE(kRuntimeError, "corrupt AST");
            }
            operation.column_type = ast->getChildren()[1]->getToken()->getString();
            break;

          default:
            RAISE(kRuntimeError, "corrupt AST");
        }

        for (size_t i = 2; i < ast->getChildren().size(); ++i) {
          switch (ast->getChildren()[i]->getType()) {
            case ASTNode::T_REPEATED:
              operation.is_repeated = true;
              break;
            case ASTNode::T_NOT_NULL:
              operation.is_optional = false;
              break;
            default:
              RAISE(kRuntimeError, "corrupt AST");
          }
        }

      return operation;
    }


    default:
      RAISE(kRuntimeError, "corrupt AST");
  }
}

QueryTreeNode* QueryPlanBuilder::buildAlterTable(
    Transaction* txn,
    ASTNode* ast) {
  if (!(*ast == ASTNode::T_ALTER_TABLE) || ast->getChildren().size() < 2) {
    return nullptr;
  }

  auto table_name = ast->getChildren()[0];
  if (table_name->getType() != ASTNode::T_TABLE_NAME ||
      table_name->getToken() == nullptr) {
    RAISE(kRuntimeError, "corrupt AST");
  }

  std::vector<AlterTableNode::AlterTableOperation> operations;
  std::vector<std::pair<std::string, std::string>> properties;
  auto child_nodes = ast->getChildren();
  for (size_t i = 1; i < child_nodes.size(); ++i) {
    auto cld = child_nodes[i];
    switch (cld->getType()) {
      //set property
      case ASTNode::T_TABLE_PROPERTY: {
        if (cld->getChildren().size() != 2) {
          RAISE(kRuntimeError, "corrupt AST");
        }

        std::pair<std::string, std::string> property;
        if (cld->getChildren().size() != 2 ||
            cld->getChildren()[0]->getType() != ASTNode::T_TABLE_PROPERTY_KEY ||
            cld->getChildren()[1]->getType() != ASTNode::T_TABLE_PROPERTY_VALUE) {
          RAISE(kRuntimeError, "corrupt AST");
        }

        std::string value;
        switch (cld->getChildren()[1]->getToken()->getType()) {
          case Token::T_TRUE:
            value = "true";
            break;
          case Token::T_FALSE:
            value = "false";
            break;
          default:
            value = cld->getChildren()[1]->getToken()->getString();
            break;
        }

        properties.emplace_back(
            cld->getChildren()[0]->getToken()->getString(),
            value);
        break;
      }

      //build operation
      default:
        operations.emplace_back(buildAlterTableOperation(cld));
        break;
    }
  }

  auto node = new AlterTableNode(
      table_name->getToken()->getString(),
      operations);
  node->setProperties(properties);

  return node;
}

}
